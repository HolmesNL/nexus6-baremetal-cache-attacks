/*
 * MODIFIED by Jan-Jaap Korpershoek in 2020 for compatibility with a bare-metal environment
 */

/*
 * Copyright 1995-2018 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

/*
 * RSA low level APIs are deprecated for public use, but still ok for
 * internal use.
 */

#include <openssl/crypto.h>
#include <openssl/engine.h>
#include <openssl/evp.h>
#include "crypto/bn.h"
#include "rsa_local.h"
#include <memory.h>

static RSA *rsa_new_intern();

#ifndef FIPS_MODE
RSA *RSA_new(void)
{
    return rsa_new_intern(NULL, NULL);
}

const RSA_METHOD *RSA_get_method(const RSA *rsa)
{
    return rsa->meth;
}

RSA *RSA_new_method(ENGINE *engine)
{
    return rsa_new_intern(engine, NULL);
}
#endif

static RSA *rsa_new_intern()
{
    RSA *ret = OPENSSL_zalloc(sizeof(*ret));
    memset(ret, 0, sizeof(*ret));

    if (ret == NULL) {
        return NULL;
    }

    ret->meth = RSA_get_default_method();
#if !defined(OPENSSL_NO_ENGINE) && !defined(FIPS_MODE)
    ret->flags = ret->meth->flags & ~RSA_FLAG_NON_FIPS_ALLOW;
    ret->engine = ENGINE_get_default_RSA();
    if (ret->engine) {
        ret->meth = ENGINE_get_RSA(ret->engine);
        if (ret->meth == NULL) {
            goto err;
        }
    }
#endif

    ret->flags = ret->meth->flags & ~RSA_FLAG_NON_FIPS_ALLOW;

    if ((ret->meth->init != NULL) && !ret->meth->init(ret)) {
        goto err;
    }

    return ret;

 err:
    return NULL;
}

void RSA_free(RSA *r)
{}
