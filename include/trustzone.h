#ifndef include_trustzone_h_INCLUDED
#define include_trustzone_h_INCLUDED

#include <scm.h>
#include <qseecomi.h>

#define KEYMASTER_NAME "keymaster"

#define QSEECOM_ALIGN_SIZE  0x40
#define QSEECOM_ALIGN_MASK  (QSEECOM_ALIGN_SIZE - 1)
#define QSEECOM_ALIGN(x)    \
    ((x + QSEECOM_ALIGN_SIZE) & (~QSEECOM_ALIGN_MASK))

void *keymaster_bin;

void set_keymaster_bin(void *_keymaster_bin);
int check_result(int ret,  struct qseecom_command_scm_resp resp);
int tz_init_sec_region(struct qseecom_command_scm_resp *resp) ;
int load_srv_img(int mdt_len, int img_len, unsigned int phy_addr, const char name[], struct qseecom_command_scm_resp *resp);
int check_trustlet_loaded(const char name[], struct qseecom_command_scm_resp *resp);
int load_trustlet(int mdt_len, int img_len, unsigned int phy_addr, const char name[], struct qseecom_command_scm_resp *resp);

#endif // include_trustzone_h_INCLUDED

