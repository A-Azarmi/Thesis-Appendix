/* crypto/frododh/frododh_lib.c based on crypto/dsa/dsa_lib.c */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 *
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 *
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 *
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

/* Original version from Steven Schoch <schoch@sheba.arc.nasa.gov> */

#include <stdio.h>
#include <stdint.h>
#include "cryptlib.h"
#include <openssl/bn.h>
#include <openssl/frododh.h>
#include <openssl/asn1.h>
#ifndef OPENSSL_NO_ENGINE
# include <openssl/engine.h>
#endif

#ifdef OPENSSL_FIPS
# include <openssl/fips.h>
#endif

const char FRODODH_version[] = "FRODODH" OPENSSL_VERSION_PTEXT;

FRODODH *FRODODH_new(void)
{
    return FRODODH_new_with_engine(NULL);
}

FRODODH *FRODODH_new_with_engine(ENGINE *engine)
{
    FRODODH *ret;

    ret = (FRODODH *)OPENSSL_malloc(sizeof(FRODODH));
    if (ret == NULL) {
        FRODODHerr(FRODODH_F_FRODODH_NEW_WITH_ENGINE, FRODODH_R_MALLOC_FAILURE);
        return (NULL);
    }
    if (engine) {
        if (!ENGINE_init(engine)) {
            FRODODHerr(FRODODH_F_FRODODH_NEW_WITH_ENGINE, FRODODH_R_ENGINE_INIT_FAILURE);
            OPENSSL_free(ret);
            return NULL;
        }
        ret->engine = engine;
    } else {

        /*
         * It appears that there is some situation in the current setup where 
         * OpenSSL_free will NOT memset this to 0. This results in some leftover
         * value that ENGINE_finish tries to use below...
         */

        ret->engine = NULL;

    }

    ret->pad = 0;
    ret->version = 0;
    ret->write_params = 1;

    ret->nid = NID_undef;
    ret->initiator = 0;
    ret->my_public_key = NULL;
    ret->parameter_set = -1;

    ret->references = 1;
    CRYPTO_new_ex_data(CRYPTO_EX_INDEX_FRODODH, ret, &ret->ex_data);
    return (ret);
}

void FRODODH_free(FRODODH *r)
{
    int i;

    if (r == NULL)
        return;

    i = CRYPTO_add(&r->references, -1, CRYPTO_LOCK_FRODODH);
#ifdef REF_PRINT
    REF_PRINT("FRODODH", r);
#endif
    if (i > 0)
        return;
#ifdef REF_CHECK
    if (i < 0) {
        fprintf(stderr, "FRODODH_free, bad reference count\n");
        abort();
    }
#endif

    if (r->engine)
        ENGINE_finish(r->engine);

    CRYPTO_free_ex_data(CRYPTO_EX_INDEX_FRODODH, r, &r->ex_data);

    if (r->my_public_key != NULL)
        ASN1_OCTET_STRING_free(r->my_public_key);

    r->initiator = 0;
    r->my_public_key = NULL;

    OPENSSL_free(r);
}

int FRODODH_up_ref(FRODODH *r)
{
    int i = CRYPTO_add(&r->references, 1, CRYPTO_LOCK_FRODODH);
#ifdef REF_PRINT
    REF_PRINT("FRODODH", r);
#endif
#ifdef REF_CHECK
    if (i < 2) {
        fprintf(stderr, "FRODODH_up_ref, bad reference count\n");
        abort();
    }
#endif
    return ((i > 1) ? 1 : 0);
}

int FRODODH_get_ex_new_index(long argl, void *argp, CRYPTO_EX_new *new_func,
                         CRYPTO_EX_dup *dup_func, CRYPTO_EX_free *free_func)
{
    return CRYPTO_get_ex_new_index(CRYPTO_EX_INDEX_FRODODH, argl, argp,
                                   new_func, dup_func, free_func);
}

int FRODODH_set_ex_data(FRODODH *d, int idx, void *arg)
{
    return (CRYPTO_set_ex_data(&d->ex_data, idx, arg));
}

void *FRODODH_get_ex_data(FRODODH *d, int idx)
{
    return (CRYPTO_get_ex_data(&d->ex_data, idx));
}
