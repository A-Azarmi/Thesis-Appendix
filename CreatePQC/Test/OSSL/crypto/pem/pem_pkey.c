/* crypto/pem/pem_pkey.c */
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

#include <stdio.h>
#include "cryptlib.h"
#include <openssl/buffer.h>
#include <openssl/objects.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/x509.h>
#include <openssl/pkcs12.h>
#include <openssl/pem.h>
#ifndef OPENSSL_NO_ENGINE
# include <openssl/engine.h>
#endif
#ifndef OPENSSL_NO_DH
# include <openssl/dh.h>
#endif
#include "asn1_locl.h"

int pem_check_suffix(const char *pem_str, const char *suffix);

static EVP_PKEY *PEM_read_bio_PrivateKey_local(BIO *bp, EVP_PKEY **x, pem_password_cb *cb,
                                               void *u, int isAlt)
{
    char *nm = NULL;
    const unsigned char *p = NULL;
    unsigned char *data = NULL;
    long len;
    int slen;
    EVP_PKEY *ret = NULL;
    char *pkcs8inf_str = NULL;
    char *pkcs_str = NULL;
    char *pem_str = NULL;

    if (isAlt) {
        pkcs8inf_str = PEM_STRING_ALT_PKCS8INF;
        pkcs_str = PEM_STRING_ALT_PKCS8;
        pem_str = PEM_STRING_EVP_ALT_PKEY;
    } else {
        pkcs8inf_str = PEM_STRING_PKCS8INF;
        pkcs_str = PEM_STRING_PKCS8;
        pem_str = PEM_STRING_EVP_PKEY;
    }

    if (!PEM_bytes_read_bio(&data, &len, &nm, pem_str, bp, cb, u))
        return NULL;
    p = data;

    if (strcmp(nm, pkcs8inf_str) == 0) {
        PKCS8_PRIV_KEY_INFO *p8inf;
        p8inf = d2i_PKCS8_PRIV_KEY_INFO(NULL, &p, len);
        if (!p8inf)
            goto p8err;
        ret = EVP_PKCS82PKEY(p8inf);
        if (x) {
            if (*x)
                EVP_PKEY_free((EVP_PKEY *)*x);
            *x = ret;
        }
        PKCS8_PRIV_KEY_INFO_free(p8inf);
    } else if (strcmp(nm, pkcs_str) == 0) {
        PKCS8_PRIV_KEY_INFO *p8inf;
        X509_SIG *p8;
        int klen;
        char psbuf[PEM_BUFSIZE];
        p8 = d2i_X509_SIG(NULL, &p, len);
        if (!p8)
            goto p8err;
        if (cb)
            klen = cb(psbuf, PEM_BUFSIZE, 0, u);
        else
            klen = PEM_def_callback(psbuf, PEM_BUFSIZE, 0, u);
        if (klen < 0) {
            PEMerr(PEM_F_PEM_READ_BIO_PRIVATEKEY_LOCAL, PEM_R_BAD_PASSWORD_READ);
            X509_SIG_free(p8);
            goto err;
        }
        p8inf = PKCS8_decrypt(p8, psbuf, klen);
        X509_SIG_free(p8);
        OPENSSL_cleanse(psbuf, klen);
        if (!p8inf)
            goto p8err;
        ret = EVP_PKCS82PKEY(p8inf);
        if (x) {
            if (*x)
                EVP_PKEY_free((EVP_PKEY *)*x);
            *x = ret;
        }
        PKCS8_PRIV_KEY_INFO_free(p8inf);
    } else if ((slen = pem_check_suffix(nm, "PRIVATE KEY")) > 0) {
        const EVP_PKEY_ASN1_METHOD *ameth;
        ameth = EVP_PKEY_asn1_find_str(NULL, nm, slen);
        if (!ameth || !ameth->old_priv_decode)
            goto p8err;
        ret = d2i_PrivateKey(ameth->pkey_id, x, &p, len);
    }
 p8err:
    if (ret == NULL)
        PEMerr(PEM_F_PEM_READ_BIO_PRIVATEKEY_LOCAL, ERR_R_ASN1_LIB);
 err:
    OPENSSL_free(nm);
    OPENSSL_cleanse(data, len);
    OPENSSL_free(data);
    return (ret);
}

EVP_PKEY *PEM_read_bio_PrivateKey(BIO *bp, EVP_PKEY **x, pem_password_cb *cb,
                                  void *u) {
    return PEM_read_bio_PrivateKey_local(bp, x, cb, u, 0);
}

EVP_PKEY *PEM_read_bio_ALTPrivateKey(BIO *bp, EVP_PKEY **x, pem_password_cb *cb,
                                     void *u) {
    return PEM_read_bio_PrivateKey_local(bp, x, cb, u, 1);
}

int PEM_write_bio_PrivateKey(BIO *bp, EVP_PKEY *x, const EVP_CIPHER *enc,
                             unsigned char *kstr, int klen,
                             pem_password_cb *cb, void *u)
{
    char pem_str[80];
    if (!x->ameth || x->ameth->priv_encode)
        return PEM_write_bio_PKCS8PrivateKey(bp, x, enc,
                                             (char *)kstr, klen, cb, u);

    BIO_snprintf(pem_str, 80, "%s PRIVATE KEY", x->ameth->pem_str);
    return PEM_ASN1_write_bio((i2d_of_void *)i2d_PrivateKey,
                              pem_str, bp, x, enc, kstr, klen, cb, u);
}

EVP_PKEY *PEM_read_bio_Parameters(BIO *bp, EVP_PKEY **x)
{
    char *nm = NULL;
    const unsigned char *p = NULL;
    unsigned char *data = NULL;
    long len;
    int slen;
    EVP_PKEY *ret = NULL;

    if (!PEM_bytes_read_bio(&data, &len, &nm, PEM_STRING_PARAMETERS,
                            bp, 0, NULL))
        return NULL;
    p = data;

    if ((slen = pem_check_suffix(nm, "PARAMETERS")) > 0) {
        ret = EVP_PKEY_new();
        if (!ret)
            goto err;
        if (!EVP_PKEY_set_type_str(ret, nm, slen)
            || !ret->ameth->param_decode
            || !ret->ameth->param_decode(ret, &p, len)) {
            EVP_PKEY_free(ret);
            ret = NULL;
            goto err;
        }
        if (x) {
            if (*x)
                EVP_PKEY_free((EVP_PKEY *)*x);
            *x = ret;
        }
    }
 err:
    if (ret == NULL)
        PEMerr(PEM_F_PEM_READ_BIO_PARAMETERS, ERR_R_ASN1_LIB);
    OPENSSL_free(nm);
    OPENSSL_free(data);
    return (ret);
}

int PEM_write_bio_Parameters(BIO *bp, EVP_PKEY *x)
{
    char pem_str[80];
    if (!x->ameth || !x->ameth->param_encode)
        return 0;

    BIO_snprintf(pem_str, 80, "%s PARAMETERS", x->ameth->pem_str);
    return PEM_ASN1_write_bio((i2d_of_void *)x->ameth->param_encode,
                              pem_str, bp, x, NULL, NULL, 0, 0, NULL);
}

#ifndef OPENSSL_NO_FP_API
EVP_PKEY *PEM_read_PrivateKey(FILE *fp, EVP_PKEY **x, pem_password_cb *cb,
                              void *u)
{
    BIO *b;
    EVP_PKEY *ret;

    if ((b = BIO_new(BIO_s_file())) == NULL) {
        PEMerr(PEM_F_PEM_READ_PRIVATEKEY, ERR_R_BUF_LIB);
        return (0);
    }
    BIO_set_fp(b, fp, BIO_NOCLOSE);
    ret = PEM_read_bio_PrivateKey(b, x, cb, u);
    BIO_free(b);
    return (ret);
}

int PEM_write_PrivateKey(FILE *fp, EVP_PKEY *x, const EVP_CIPHER *enc,
                         unsigned char *kstr, int klen,
                         pem_password_cb *cb, void *u)
{
    BIO *b;
    int ret;

    if ((b = BIO_new_fp(fp, BIO_NOCLOSE)) == NULL) {
        PEMerr(PEM_F_PEM_WRITE_PRIVATEKEY, ERR_R_BUF_LIB);
        return 0;
    }
    ret = PEM_write_bio_PrivateKey(b, x, enc, kstr, klen, cb, u);
    BIO_free(b);
    return ret;
}

#endif

#ifndef OPENSSL_NO_DH

/* Transparently read in PKCS#3 or X9.42 DH parameters */

DH *PEM_read_bio_DHparams(BIO *bp, DH **x, pem_password_cb *cb, void *u)
{
    char *nm = NULL;
    const unsigned char *p = NULL;
    unsigned char *data = NULL;
    long len;
    DH *ret = NULL;

    if (!PEM_bytes_read_bio(&data, &len, &nm, PEM_STRING_DHPARAMS, bp, cb, u))
        return NULL;
    p = data;

    if (!strcmp(nm, PEM_STRING_DHXPARAMS))
        ret = d2i_DHxparams(x, &p, len);
    else
        ret = d2i_DHparams(x, &p, len);

    if (ret == NULL)
        PEMerr(PEM_F_PEM_READ_BIO_DHPARAMS, ERR_R_ASN1_LIB);
    OPENSSL_free(nm);
    OPENSSL_free(data);
    return ret;
}

# ifndef OPENSSL_NO_FP_API
DH *PEM_read_DHparams(FILE *fp, DH **x, pem_password_cb *cb, void *u)
{
    BIO *b;
    DH *ret;

    if ((b = BIO_new(BIO_s_file())) == NULL) {
        PEMerr(PEM_F_PEM_READ_DHPARAMS, ERR_R_BUF_LIB);
        return (0);
    }
    BIO_set_fp(b, fp, BIO_NOCLOSE);
    ret = PEM_read_bio_DHparams(b, x, cb, u);
    BIO_free(b);
    return (ret);
}
# endif

#endif
