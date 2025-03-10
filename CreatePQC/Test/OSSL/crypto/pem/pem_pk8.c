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

static int do_pk8pkey(BIO *bp, EVP_PKEY *x, int isder,
                      int nid, const EVP_CIPHER *enc,
                      char *kstr, int klen, pem_password_cb *cb, void *u, int isalt);
static int do_pk8pkey_fp(FILE *bp, EVP_PKEY *x, int isder,
                         int nid, const EVP_CIPHER *enc,
                         char *kstr, int klen, pem_password_cb *cb, void *u);


IMPLEMENT_PEM_rw(ALT_PKCS8, X509_SIG, PEM_STRING_ALT_PKCS8, X509_SIG)
IMPLEMENT_PEM_rw(ALT_PKCS8_PRIV_KEY_INFO, PKCS8_PRIV_KEY_INFO, PEM_STRING_ALT_PKCS8INF,
             PKCS8_PRIV_KEY_INFO)

/*
 * These functions write a private key in PKCS#8 format: it is a "drop in"
 * replacement for PEM_write_bio_PrivateKey() and friends. As usual if 'enc'
 * is NULL then it uses the unencrypted private key form. The 'nid' versions
 * uses PKCS#5 v1.5 PBE algorithms whereas the others use PKCS#5 v2.0.
 */

int PEM_write_bio_PKCS8PrivateKey_nid(BIO *bp, EVP_PKEY *x, int nid,
                                      char *kstr, int klen,
                                      pem_password_cb *cb, void *u)
{
    return do_pk8pkey(bp, x, 0, nid, NULL, kstr, klen, cb, u, 0);
}

int PEM_write_bio_PKCS8PrivateKey(BIO *bp, EVP_PKEY *x, const EVP_CIPHER *enc,
                                  char *kstr, int klen,
                                  pem_password_cb *cb, void *u)
{
    return do_pk8pkey(bp, x, 0, -1, enc, kstr, klen, cb, u, 0);
}

int PEM_write_bio_ALT_PKCS8PrivateKey(BIO *bp, EVP_PKEY *x, const EVP_CIPHER *enc,
                                      char *kstr, int klen,
                                      pem_password_cb *cb, void *u)
{
    return do_pk8pkey(bp, x, 0, -1, enc, kstr, klen, cb, u, 1);
}


int i2d_PKCS8PrivateKey_bio(BIO *bp, EVP_PKEY *x, const EVP_CIPHER *enc,
                            char *kstr, int klen,
                            pem_password_cb *cb, void *u)
{
    return do_pk8pkey(bp, x, 1, -1, enc, kstr, klen, cb, u, 0);
}

int i2d_PKCS8PrivateKey_nid_bio(BIO *bp, EVP_PKEY *x, int nid,
                                char *kstr, int klen,
                                pem_password_cb *cb, void *u)
{
    return do_pk8pkey(bp, x, 1, nid, NULL, kstr, klen, cb, u, 0);
}

static int do_pk8pkey(BIO *bp, EVP_PKEY *x, int isder, int nid,
                      const EVP_CIPHER *enc, char *kstr, int klen,
                      pem_password_cb *cb, void *u, int isalt)
{
    X509_SIG *p8;
    PKCS8_PRIV_KEY_INFO *p8inf;
    char buf[PEM_BUFSIZE];
    int ret;
    if (!(p8inf = EVP_PKEY2PKCS8(x))) {
        PEMerr(PEM_F_DO_PK8PKEY, PEM_R_ERROR_CONVERTING_PRIVATE_KEY);
        return 0;
    }
    if (enc || (nid != -1)) {
        if (!kstr) {
            if (!cb)
                klen = PEM_def_callback(buf, PEM_BUFSIZE, 1, u);
            else
                klen = cb(buf, PEM_BUFSIZE, 1, u);
            if (klen <= 0) {
                PEMerr(PEM_F_DO_PK8PKEY, PEM_R_READ_KEY);
                PKCS8_PRIV_KEY_INFO_free(p8inf);
                return 0;
            }

            kstr = buf;
        }
        p8 = PKCS8_encrypt(nid, enc, kstr, klen, NULL, 0, 0, p8inf);
        if (kstr == buf)
            OPENSSL_cleanse(buf, klen);
        PKCS8_PRIV_KEY_INFO_free(p8inf);
        if (p8 == NULL)
            return 0;
        if (isder) {
            if (isalt) {
                return 0;
            } else {
                ret = i2d_PKCS8_bio(bp, p8);
            }
        } else {
            if (isalt) {
                ret = PEM_write_bio_ALT_PKCS8(bp, p8);
            } else {
                ret = PEM_write_bio_PKCS8(bp, p8);
            }
        }
        X509_SIG_free(p8);
        return ret;
    } else {
        if (isder) {
            if (isalt) {
                return 0;
            } else {
                ret = i2d_PKCS8_PRIV_KEY_INFO_bio(bp, p8inf);
            }
        } else {
            if (isalt) {
                ret = PEM_write_bio_ALT_PKCS8_PRIV_KEY_INFO(bp, p8inf);
            } else {
                ret = PEM_write_bio_PKCS8_PRIV_KEY_INFO(bp, p8inf);
            }
        }
        PKCS8_PRIV_KEY_INFO_free(p8inf);
        return ret;
    }
}

EVP_PKEY *d2i_PKCS8PrivateKey_bio(BIO *bp, EVP_PKEY **x, pem_password_cb *cb,
                                  void *u)
{
    PKCS8_PRIV_KEY_INFO *p8inf = NULL;
    X509_SIG *p8 = NULL;
    int klen;
    EVP_PKEY *ret;
    char psbuf[PEM_BUFSIZE];
    p8 = d2i_PKCS8_bio(bp, NULL);
    if (!p8)
        return NULL;
    if (cb)
        klen = cb(psbuf, PEM_BUFSIZE, 0, u);
    else
        klen = PEM_def_callback(psbuf, PEM_BUFSIZE, 0, u);
    if (klen < 0) {
        PEMerr(PEM_F_D2I_PKCS8PRIVATEKEY_BIO, PEM_R_BAD_PASSWORD_READ);
        X509_SIG_free(p8);
        return NULL;
    }
    p8inf = PKCS8_decrypt(p8, psbuf, klen);
    X509_SIG_free(p8);
    OPENSSL_cleanse(psbuf, klen);
    if (!p8inf)
        return NULL;
    ret = EVP_PKCS82PKEY(p8inf);
    PKCS8_PRIV_KEY_INFO_free(p8inf);
    if (!ret)
        return NULL;
    if (x) {
        if (*x)
            EVP_PKEY_free(*x);
        *x = ret;
    }
    return ret;
}

#ifndef OPENSSL_NO_FP_API

int i2d_PKCS8PrivateKey_fp(FILE *fp, EVP_PKEY *x, const EVP_CIPHER *enc,
                           char *kstr, int klen, pem_password_cb *cb, void *u)
{
    return do_pk8pkey_fp(fp, x, 1, -1, enc, kstr, klen, cb, u);
}

int i2d_PKCS8PrivateKey_nid_fp(FILE *fp, EVP_PKEY *x, int nid,
                               char *kstr, int klen,
                               pem_password_cb *cb, void *u)
{
    return do_pk8pkey_fp(fp, x, 1, nid, NULL, kstr, klen, cb, u);
}

int PEM_write_PKCS8PrivateKey_nid(FILE *fp, EVP_PKEY *x, int nid,
                                  char *kstr, int klen,
                                  pem_password_cb *cb, void *u)
{
    return do_pk8pkey_fp(fp, x, 0, nid, NULL, kstr, klen, cb, u);
}

int PEM_write_PKCS8PrivateKey(FILE *fp, EVP_PKEY *x, const EVP_CIPHER *enc,
                              char *kstr, int klen, pem_password_cb *cb,
                              void *u)
{
    return do_pk8pkey_fp(fp, x, 0, -1, enc, kstr, klen, cb, u);
}

static int do_pk8pkey_fp(FILE *fp, EVP_PKEY *x, int isder, int nid,
                         const EVP_CIPHER *enc, char *kstr, int klen,
                         pem_password_cb *cb, void *u)
{
    BIO *bp;
    int ret;
    if (!(bp = BIO_new_fp(fp, BIO_NOCLOSE))) {
        PEMerr(PEM_F_DO_PK8PKEY_FP, ERR_R_BUF_LIB);
        return (0);
    }
    ret = do_pk8pkey(bp, x, isder, nid, enc, kstr, klen, cb, u, 0);
    BIO_free(bp);
    return ret;
}

EVP_PKEY *d2i_PKCS8PrivateKey_fp(FILE *fp, EVP_PKEY **x, pem_password_cb *cb,
                                 void *u)
{
    BIO *bp;
    EVP_PKEY *ret;
    if (!(bp = BIO_new_fp(fp, BIO_NOCLOSE))) {
        PEMerr(PEM_F_D2I_PKCS8PRIVATEKEY_FP, ERR_R_BUF_LIB);
        return NULL;
    }
    ret = d2i_PKCS8PrivateKey_bio(bp, x, cb, u);
    BIO_free(bp);
    return ret;
}

#endif

IMPLEMENT_PEM_rw(PKCS8, X509_SIG, PEM_STRING_PKCS8, X509_SIG)


IMPLEMENT_PEM_rw(PKCS8_PRIV_KEY_INFO, PKCS8_PRIV_KEY_INFO, PEM_STRING_PKCS8INF,
             PKCS8_PRIV_KEY_INFO)
