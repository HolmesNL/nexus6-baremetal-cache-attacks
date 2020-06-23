#include <trustzone.h>
#include <serial.h>
#include <printf.h>
#include <memory.h>

int check_result(int ret,  struct qseecom_command_scm_resp resp)
{
	struct msm_serial_data dev;
	dev.base = MSM_SERIAL_BASE;
	char buf[64];
	
	if (ret >= 0 && resp.result == QSEOS_RESULT_SUCCESS) {
    	print("Success!\r\n");
		return 0;
    }
    else {
        print("Fail! ret:%d, TZ_resp: %08x\r\n", ret, resp.result);
	    if(ret) {
	       return ret;
    	}
    	else if(resp.result) {
        	return resp.result;
    	}
    return -1;
	}
}

/* Setup the region in RAM that is reserved for TZ, then notify TZ of the location and size.
   If this is not done, TZ will only be able to execute simple calls (presumably those implemented in the kernel),
   like checking if a trustlet exists, but it will not be able to e.g. load trustlets, since it lacks the
   memory to do so. If this is attempted, this gives an error -41 (0xFFFFFFD7).
*/
int tz_init_sec_region(struct qseecom_command_scm_resp *resp) 
{	
	struct qsee_apps_region_info_ireq req;
	memset(resp, 0, sizeof(struct qseecom_command_scm_resp));
	int rc = 0;

	req.qsee_cmd_id = QSEOS_APP_REGION_NOTIFICATION;
	req.addr = 0xd600000;
	req.size = 0x500000;

	return scm_call(SCM_SVC_TZSCHEDULER, 1, &req, sizeof(req), resp, sizeof(struct qseecom_command_scm_resp));
}


/* This function is used to load the cmnlib trustlet, which seems to contain all kinds of function that can be used
by trustlets (kind of a libc-like functionality). Why this has to be loaded with a different call is not entirely clear
to me, but it seems to have something to do with not being able to load mulitple trustlets at once (haven't verified this), 
while this library of course has to be loaded at the same time as a trustlet. */
int load_srv_img(int mdt_len, int img_len, unsigned int phy_addr, const char name[], struct qseecom_command_scm_resp *resp)
{	
	struct qseecom_load_app_ireq load_req;	
	int ret = -1;	
	memset(resp, 0, sizeof(struct qseecom_command_scm_resp));
	memset(&load_req.app_name, 0, 32);

	load_req.qsee_cmd_id = QSEOS_LOAD_SERV_IMAGE_COMMAND;	
	load_req.mdt_len = mdt_len;
	load_req.img_len = img_len;
	load_req.phy_addr = phy_addr;
	memcpy(load_req.app_name, name, 31);
		
	ret = scm_call(SCM_SVC_TZSCHEDULER, 1,  &load_req,
			sizeof(struct qseecom_load_app_ireq),
			resp, sizeof(struct qseecom_command_scm_resp));
	if(ret) {
		return ret;
	}
	else if(resp->result) {
		return resp->result;
	}
	else {
		return resp->data;
	}
}

int check_trustlet_loaded(const char name[], struct qseecom_command_scm_resp *resp)
{
	struct qseecom_check_app_ireq req;
	memset(req.app_name, 0, 32);
    memset(resp, 0, sizeof(struct qseecom_command_scm_resp));
	int ret = -1;

	req.qsee_cmd_id = QSEOS_APP_LOOKUP_COMMAND;
	memcpy(req.app_name, name, 31);
	
	ret = scm_call(SCM_SVC_TZSCHEDULER, 1,  &req,
			sizeof(struct qseecom_check_app_ireq),
			resp, sizeof(struct qseecom_command_scm_resp));
	
    if(ret) {
        return ret;
    }
    else if(resp->result) {
        return resp->result;
    }
    else {
        return resp->data;
    }	
}

/* This function actually loads the trustlet. Before this is called, QSEEOS needs to be notified of which RAM region
 is reserved for it (using the tz_init_sec_region function), otherwise this call will always fail */
int load_trustlet(int mdt_len, int img_len, unsigned int phy_addr, const char name[], struct qseecom_command_scm_resp *resp)
{	
	struct qseecom_load_app_ireq load_req;	
	int ret = -1;	
	memset(&load_req.app_name, 0, 32);
	memset(resp, 0, sizeof(struct qseecom_command_scm_resp));

	load_req.qsee_cmd_id = QSEOS_APP_START_COMMAND;
					
	load_req.mdt_len = mdt_len;
	load_req.img_len = img_len;
	load_req.phy_addr = phy_addr;
	memcpy(load_req.app_name, name, 31);

	ret = scm_call(SCM_SVC_TZSCHEDULER, 1,  &load_req,
				sizeof(struct qseecom_load_app_ireq),
				resp, sizeof(struct qseecom_command_scm_resp));

	if(ret) {
		return ret;
	}
	else if(resp->result) {
		return resp->result;
	}
	else {
		return resp->data;
	}
}

void set_keymaster_bin(void *_keymaster_bin)
{
    keymaster_bin = _keymaster_bin;
}
