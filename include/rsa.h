#ifndef include_rsa_h_INCLUDED
#define include_rsa_h_INCLUDED

#include <types.h>
#include <openssl/rsa.h>
#include <serial.h>

RSA *rsa_keygen(int preset);
void rsa_test(RSA* rsa);

#endif // include_rsa_h_INCLUDED

