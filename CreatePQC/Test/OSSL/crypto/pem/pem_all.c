/* crypto/pem/pem_all.c */
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
/* ====================================================================
 * Copyright (c) 1998-2002 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.openssl.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    openssl-core@openssl.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.openssl.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This product includes cryptographic software written by Eric Young
 * (eay@cryptsoft.com).  This product includes software written by Tim
 * Hudson (tjh@cryptsoft.com).
 *
 */

#include <stdio.h>
#include "cryptlib.h"
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/pkcs7.h>
#include <openssl/pem.h>
#ifndef OPENSSL_NO_RSA
# include <openssl/rsa.h>
#endif
#ifndef OPENSSL_NO_DSA
# include <openssl/dsa.h>
#endif
#ifndef OPENSSL_NO_DH
# include <openssl/dh.h>
#endif
#ifndef OPENSSL_NO_HSS
# include <openssl/hss.h>
#endif
#ifndef OPENSSL_NO_XMSS
# include <openssl/xmss.h>
#endif
#ifndef OPENSSL_NO_XMSSMT
# include <openssl/xmssmt.h>
#endif
#ifndef OPENSSL_NO_DILITHIUM
# include <openssl/dilithium.h>
#endif
#ifndef OPENSSL_NO_SPHINCS
# include <openssl/sphincs.h>
#endif
#ifndef OPENSSL_NO_RAINBOW
# include <openssl/rainbow.h>
#endif

#ifndef OPENSSL_NO_RSA
static RSA *pkey_get_rsa(EVP_PKEY *key, RSA **rsa);
#endif
#ifndef OPENSSL_NO_DSA
static DSA *pkey_get_dsa(EVP_PKEY *key, DSA **dsa);
#endif

#ifndef OPENSSL_NO_EC
static EC_KEY *pkey_get_eckey(EVP_PKEY *key, EC_KEY **eckey);
#endif

IMPLEMENT_PEM_rw(X509_REQ, X509_REQ, PEM_STRING_X509_REQ, X509_REQ)

IMPLEMENT_PEM_write(X509_REQ_NEW, X509_REQ, PEM_STRING_X509_REQ_OLD, X509_REQ)
IMPLEMENT_PEM_rw(X509_CRL, X509_CRL, PEM_STRING_X509_CRL, X509_CRL)
IMPLEMENT_PEM_rw(PKCS7, PKCS7, PEM_STRING_PKCS7, PKCS7)

IMPLEMENT_PEM_rw(NETSCAPE_CERT_SEQUENCE, NETSCAPE_CERT_SEQUENCE,
                 PEM_STRING_X509, NETSCAPE_CERT_SEQUENCE)
#ifndef OPENSSL_NO_RSA
/*
 * We treat RSA or DSA private keys as a special case. For private keys we
 * read in an EVP_PKEY structure with PEM_read_bio_PrivateKey() and extract
 * the relevant private key: this means can handle "traditional" and PKCS#8
 * formats transparently.
 */
static RSA *pkey_get_rsa(EVP_PKEY *key, RSA **rsa)
{
    RSA *rtmp;
    if (!key)
        return NULL;
    rtmp = EVP_PKEY_get1_RSA(key);
    EVP_PKEY_free(key);
    if (!rtmp)
        return NULL;
    if (rsa) {
        RSA_free(*rsa);
        *rsa = rtmp;
    }
    return rtmp;
}

RSA *PEM_read_bio_RSAPrivateKey(BIO *bp, RSA **rsa, pem_password_cb *cb,
                                void *u)
{
    EVP_PKEY *pktmp;
    pktmp = PEM_read_bio_PrivateKey(bp, NULL, cb, u);
    return pkey_get_rsa(pktmp, rsa);
}

# ifndef OPENSSL_NO_FP_API

RSA *PEM_read_RSAPrivateKey(FILE *fp, RSA **rsa, pem_password_cb *cb, void *u)
{
    EVP_PKEY *pktmp;
    pktmp = PEM_read_PrivateKey(fp, NULL, cb, u);
    return pkey_get_rsa(pktmp, rsa);
}

# endif

# ifdef OPENSSL_FIPS

int PEM_write_bio_RSAPrivateKey(BIO *bp, RSA *x, const EVP_CIPHER *enc,
                                unsigned char *kstr, int klen,
                                pem_password_cb *cb, void *u)
{
    if (FIPS_mode()) {
        EVP_PKEY *k;
        int ret;
        k = EVP_PKEY_new();
        if (!k)
            return 0;
        EVP_PKEY_set1_RSA(k, x);

        ret = PEM_write_bio_PrivateKey(bp, k, enc, kstr, klen, cb, u);
        EVP_PKEY_free(k);
        return ret;
    } else
        return PEM_ASN1_write_bio((i2d_of_void *)i2d_RSAPrivateKey,
                                  PEM_STRING_RSA, bp, x, enc, kstr, klen, cb,
                                  u);
}

#  ifndef OPENSSL_NO_FP_API
int PEM_write_RSAPrivateKey(FILE *fp, RSA *x, const EVP_CIPHER *enc,
                            unsigned char *kstr, int klen,
                            pem_password_cb *cb, void *u)
{
    if (FIPS_mode()) {
        EVP_PKEY *k;
        int ret;
        k = EVP_PKEY_new();
        if (!k)
            return 0;

        EVP_PKEY_set1_RSA(k, x);

        ret = PEM_write_PrivateKey(fp, k, enc, kstr, klen, cb, u);
        EVP_PKEY_free(k);
        return ret;
    } else
        return PEM_ASN1_write((i2d_of_void *)i2d_RSAPrivateKey,
                              PEM_STRING_RSA, fp, x, enc, kstr, klen, cb, u);
}
#  endif

# else

IMPLEMENT_PEM_write_cb_const(RSAPrivateKey, RSA, PEM_STRING_RSA,
                             RSAPrivateKey)
# endif
IMPLEMENT_PEM_rw_const(RSAPublicKey, RSA, PEM_STRING_RSA_PUBLIC,
                       RSAPublicKey) IMPLEMENT_PEM_rw(RSA_PUBKEY, RSA,
                                                      PEM_STRING_PUBLIC,
                                                      RSA_PUBKEY)
#endif
#ifndef OPENSSL_NO_DSA
static DSA *pkey_get_dsa(EVP_PKEY *key, DSA **dsa)
{
    DSA *dtmp;
    if (!key)
        return NULL;
    dtmp = EVP_PKEY_get1_DSA(key);
    EVP_PKEY_free(key);
    if (!dtmp)
        return NULL;
    if (dsa) {
        DSA_free(*dsa);
        *dsa = dtmp;
    }
    return dtmp;
}

DSA *PEM_read_bio_DSAPrivateKey(BIO *bp, DSA **dsa, pem_password_cb *cb,
                                void *u)
{
    EVP_PKEY *pktmp;
    pktmp = PEM_read_bio_PrivateKey(bp, NULL, cb, u);
    return pkey_get_dsa(pktmp, dsa); /* will free pktmp */
}

# ifdef OPENSSL_FIPS

int PEM_write_bio_DSAPrivateKey(BIO *bp, DSA *x, const EVP_CIPHER *enc,
                                unsigned char *kstr, int klen,
                                pem_password_cb *cb, void *u)
{
    if (FIPS_mode()) {
        EVP_PKEY *k;
        int ret;
        k = EVP_PKEY_new();
        if (!k)
            return 0;
        EVP_PKEY_set1_DSA(k, x);

        ret = PEM_write_bio_PrivateKey(bp, k, enc, kstr, klen, cb, u);
        EVP_PKEY_free(k);
        return ret;
    } else
        return PEM_ASN1_write_bio((i2d_of_void *)i2d_DSAPrivateKey,
                                  PEM_STRING_DSA, bp, x, enc, kstr, klen, cb,
                                  u);
}

#  ifndef OPENSSL_NO_FP_API
int PEM_write_DSAPrivateKey(FILE *fp, DSA *x, const EVP_CIPHER *enc,
                            unsigned char *kstr, int klen,
                            pem_password_cb *cb, void *u)
{
    if (FIPS_mode()) {
        EVP_PKEY *k;
        int ret;
        k = EVP_PKEY_new();
        if (!k)
            return 0;
        EVP_PKEY_set1_DSA(k, x);
        ret = PEM_write_PrivateKey(fp, k, enc, kstr, klen, cb, u);
        EVP_PKEY_free(k);
        return ret;
    } else
        return PEM_ASN1_write((i2d_of_void *)i2d_DSAPrivateKey,
                              PEM_STRING_DSA, fp, x, enc, kstr, klen, cb, u);
}
#  endif

# else

IMPLEMENT_PEM_write_cb_const(DSAPrivateKey, DSA, PEM_STRING_DSA,
                             DSAPrivateKey)
# endif
    IMPLEMENT_PEM_rw(DSA_PUBKEY, DSA, PEM_STRING_PUBLIC, DSA_PUBKEY)
# ifndef OPENSSL_NO_FP_API
DSA *PEM_read_DSAPrivateKey(FILE *fp, DSA **dsa, pem_password_cb *cb, void *u)
{
    EVP_PKEY *pktmp;
    pktmp = PEM_read_PrivateKey(fp, NULL, cb, u);
    return pkey_get_dsa(pktmp, dsa); /* will free pktmp */
}

# endif

IMPLEMENT_PEM_rw_const(DSAparams, DSA, PEM_STRING_DSAPARAMS, DSAparams)
#endif

#ifndef OPENSSL_NO_HSS
static HSS *pkey_get_hss(EVP_PKEY *key, HSS **hss)
{
    HSS *ltmp;
    if (!key)
        return NULL;
    ltmp = EVP_PKEY_get1_HSS(key);
    EVP_PKEY_free(key);
    if (!ltmp)
        return NULL;
    if (hss) {
        HSS_free(*hss);
        *hss = ltmp;
    }
    return ltmp;
}

HSS *PEM_read_bio_HSSPrivateKey(BIO *bp, HSS **hss, pem_password_cb *cb,
                                void *u)
{
    EVP_PKEY *pktmp;
    pktmp = PEM_read_bio_PrivateKey(bp, NULL, cb, u);
    return pkey_get_hss(pktmp, hss); /* will free pktmp */
}

IMPLEMENT_PEM_write_cb_const(HSSPrivateKey, HSS, PEM_STRING_HSS,
                             HSSPrivateKey)

    IMPLEMENT_PEM_rw(HSS_PUBKEY, HSS, PEM_STRING_PUBLIC, HSS_PUBKEY)
# ifndef OPENSSL_NO_FP_API
HSS *PEM_read_HSSPrivateKey(FILE *fp, HSS **hss, pem_password_cb *cb, void *u)
{
    EVP_PKEY *pktmp;
    pktmp = PEM_read_PrivateKey(fp, NULL, cb, u);
    return pkey_get_hss(pktmp, hss); /* will free pktmp */
}

# endif
#endif

#ifndef OPENSSL_NO_XMSS
static XMSS *pkey_get_xmss(EVP_PKEY *key, XMSS **xmss)
{
    XMSS *ltmp;
    if (!key)
        return NULL;
    ltmp = EVP_PKEY_get1_XMSS(key);
    EVP_PKEY_free(key);
    if (!ltmp)
        return NULL;
    if (xmss) {
        XMSS_free(*xmss);
        *xmss = ltmp;
    }
    return ltmp;
}

XMSS *PEM_read_bio_XMSSPrivateKey(BIO *bp, XMSS **xmss, pem_password_cb *cb,
                                void *u)
{
    EVP_PKEY *pktmp;
    pktmp = PEM_read_bio_PrivateKey(bp, NULL, cb, u);
    return pkey_get_xmss(pktmp, xmss); /* will free pktmp */
}

IMPLEMENT_PEM_write_cb_const(XMSSPrivateKey, XMSS, PEM_STRING_XMSS,
                             XMSSPrivateKey)

    IMPLEMENT_PEM_rw(XMSS_PUBKEY, XMSS, PEM_STRING_PUBLIC, XMSS_PUBKEY)
# ifndef OPENSSL_NO_FP_API
XMSS *PEM_read_XMSSPrivateKey(FILE *fp, XMSS **xmss, pem_password_cb *cb, void *u)
{
    EVP_PKEY *pktmp;
    pktmp = PEM_read_PrivateKey(fp, NULL, cb, u);
    return pkey_get_xmss(pktmp, xmss); /* will free pktmp */
}

# endif
#endif

#ifndef OPENSSL_NO_XMSSMT
static XMSSMT *pkey_get_xmssmt(EVP_PKEY *key, XMSSMT **xmssmt)
{
    XMSSMT *ltmp;
    if (!key)
        return NULL;
    ltmp = EVP_PKEY_get1_XMSSMT(key);
    EVP_PKEY_free(key);
    if (!ltmp)
        return NULL;
    if (xmssmt) {
        XMSSMT_free(*xmssmt);
        *xmssmt = ltmp;
    }
    return ltmp;
}

XMSSMT *PEM_read_bio_XMSSMTPrivateKey(BIO *bp, XMSSMT **xmssmt, pem_password_cb *cb,
                                void *u)
{
    EVP_PKEY *pktmp;
    pktmp = PEM_read_bio_PrivateKey(bp, NULL, cb, u);
    return pkey_get_xmssmt(pktmp, xmssmt); /* will free pktmp */
}

IMPLEMENT_PEM_write_cb_const(XMSSMTPrivateKey, XMSSMT, PEM_STRING_XMSSMT,
                             XMSSMTPrivateKey)

    IMPLEMENT_PEM_rw(XMSSMT_PUBKEY, XMSSMT, PEM_STRING_PUBLIC, XMSSMT_PUBKEY)
# ifndef OPENSSL_NO_FP_API
XMSSMT *PEM_read_XMSSMTPrivateKey(FILE *fp, XMSSMT **xmssmt, pem_password_cb *cb, void *u)
{
    EVP_PKEY *pktmp;
    pktmp = PEM_read_PrivateKey(fp, NULL, cb, u);
    return pkey_get_xmssmt(pktmp, xmssmt); /* will free pktmp */
}

# endif

#endif

#ifndef OPENSSL_NO_DILITHIUM
static DILITHIUM *pkey_get_dilithium(EVP_PKEY *key, DILITHIUM **dilithium)
{
    DILITHIUM *ltmp;
    if (!key)
        return NULL;
    ltmp = EVP_PKEY_get1_DILITHIUM(key);
    EVP_PKEY_free(key);
    if (!ltmp)
        return NULL;
    if (dilithium) {
        DILITHIUM_free(*dilithium);
        *dilithium = ltmp;
    }
    return ltmp;
}

DILITHIUM *PEM_read_bio_DILITHIUMPrivateKey(BIO *bp, DILITHIUM **dilithium, pem_password_cb *cb,
                                void *u)
{
    EVP_PKEY *pktmp;
    pktmp = PEM_read_bio_PrivateKey(bp, NULL, cb, u);
    return pkey_get_dilithium(pktmp, dilithium); /* will free pktmp */
}

IMPLEMENT_PEM_write_cb_const(DILITHIUMPrivateKey, DILITHIUM, PEM_STRING_DILITHIUM,
                             DILITHIUMPrivateKey)

    IMPLEMENT_PEM_rw(DILITHIUM_PUBKEY, DILITHIUM, PEM_STRING_PUBLIC, DILITHIUM_PUBKEY)
# ifndef OPENSSL_NO_FP_API
DILITHIUM *PEM_read_DILITHIUMPrivateKey(FILE *fp, DILITHIUM **dilithium, pem_password_cb *cb, void *u)
{
    EVP_PKEY *pktmp;
    pktmp = PEM_read_PrivateKey(fp, NULL, cb, u);
    return pkey_get_dilithium(pktmp, dilithium); /* will free pktmp */
}

# endif
#endif

#ifndef OPENSSL_NO_SPHINCS
static SPHINCS *pkey_get_sphincs(EVP_PKEY *key, SPHINCS **sphincs)
{
    SPHINCS *ltmp;
    if (!key)
        return NULL;
    ltmp = EVP_PKEY_get1_SPHINCS(key);
    EVP_PKEY_free(key);
    if (!ltmp)
        return NULL;
    if (sphincs) {
        SPHINCS_free(*sphincs);
        *sphincs = ltmp;
    }
    return ltmp;
}

SPHINCS *PEM_read_bio_SPHINCSPrivateKey(BIO *bp, SPHINCS **sphincs, pem_password_cb *cb,
                                void *u)
{
    EVP_PKEY *pktmp;
    pktmp = PEM_read_bio_PrivateKey(bp, NULL, cb, u);
    return pkey_get_sphincs(pktmp, sphincs); /* will free pktmp */
}

IMPLEMENT_PEM_write_cb_const(SPHINCSPrivateKey, SPHINCS, PEM_STRING_SPHINCS,
                             SPHINCSPrivateKey)

    IMPLEMENT_PEM_rw(SPHINCS_PUBKEY, SPHINCS, PEM_STRING_PUBLIC, SPHINCS_PUBKEY)
# ifndef OPENSSL_NO_FP_API
SPHINCS *PEM_read_SPHINCSPrivateKey(FILE *fp, SPHINCS **sphincs, pem_password_cb *cb, void *u)
{
    EVP_PKEY *pktmp;
    pktmp = PEM_read_PrivateKey(fp, NULL, cb, u);
    return pkey_get_sphincs(pktmp, sphincs); /* will free pktmp */
}

# endif
#endif

#ifndef OPENSSL_NO_RAINBOW
static RAINBOW *pkey_get_rainbow(EVP_PKEY *key, RAINBOW **rainbow)
{
    RAINBOW *ltmp;
    if (!key)
        return NULL;
    ltmp = EVP_PKEY_get1_RAINBOW(key);
    EVP_PKEY_free(key);
    if (!ltmp)
        return NULL;
    if (rainbow) {
        RAINBOW_free(*rainbow);
        *rainbow = ltmp;
    }
    return ltmp;
}

RAINBOW *PEM_read_bio_RAINBOWPrivateKey(BIO *bp, RAINBOW **rainbow, pem_password_cb *cb,
                                void *u)
{
    EVP_PKEY *pktmp;
    pktmp = PEM_read_bio_PrivateKey(bp, NULL, cb, u);
    return pkey_get_rainbow(pktmp, rainbow); /* will free pktmp */
}

IMPLEMENT_PEM_write_cb_const(RAINBOWPrivateKey, RAINBOW, PEM_STRING_RAINBOW,
                             RAINBOWPrivateKey)

    IMPLEMENT_PEM_rw(RAINBOW_PUBKEY, RAINBOW, PEM_STRING_PUBLIC, RAINBOW_PUBKEY)
# ifndef OPENSSL_NO_FP_API
RAINBOW *PEM_read_RAINBOWPrivateKey(FILE *fp, RAINBOW **rainbow, pem_password_cb *cb, void *u)
{
    EVP_PKEY *pktmp;
    pktmp = PEM_read_PrivateKey(fp, NULL, cb, u);
    return pkey_get_rainbow(pktmp, rainbow); /* will free pktmp */
}

# endif

#endif

#ifndef OPENSSL_NO_EC
static EC_KEY *pkey_get_eckey(EVP_PKEY *key, EC_KEY **eckey)
{
    EC_KEY *dtmp;
    if (!key)
        return NULL;
    dtmp = EVP_PKEY_get1_EC_KEY(key);
    EVP_PKEY_free(key);
    if (!dtmp)
        return NULL;
    if (eckey) {
        EC_KEY_free(*eckey);
        *eckey = dtmp;
    }
    return dtmp;
}

EC_KEY *PEM_read_bio_ECPrivateKey(BIO *bp, EC_KEY **key, pem_password_cb *cb,
                                  void *u)
{
    EVP_PKEY *pktmp;
    pktmp = PEM_read_bio_PrivateKey(bp, NULL, cb, u);
    return pkey_get_eckey(pktmp, key); /* will free pktmp */
}

IMPLEMENT_PEM_rw_const(ECPKParameters, EC_GROUP, PEM_STRING_ECPARAMETERS,
                       ECPKParameters)
# ifdef OPENSSL_FIPS
int PEM_write_bio_ECPrivateKey(BIO *bp, EC_KEY *x, const EVP_CIPHER *enc,
                               unsigned char *kstr, int klen,
                               pem_password_cb *cb, void *u)
{
    if (FIPS_mode()) {
        EVP_PKEY *k;
        int ret;
        k = EVP_PKEY_new();
        if (!k)
            return 0;
        EVP_PKEY_set1_EC_KEY(k, x);

        ret = PEM_write_bio_PrivateKey(bp, k, enc, kstr, klen, cb, u);
        EVP_PKEY_free(k);
        return ret;
    } else
        return PEM_ASN1_write_bio((i2d_of_void *)i2d_ECPrivateKey,
                                  PEM_STRING_ECPRIVATEKEY,
                                  bp, x, enc, kstr, klen, cb, u);
}

#  ifndef OPENSSL_NO_FP_API
int PEM_write_ECPrivateKey(FILE *fp, EC_KEY *x, const EVP_CIPHER *enc,
                           unsigned char *kstr, int klen,
                           pem_password_cb *cb, void *u)
{
    if (FIPS_mode()) {
        EVP_PKEY *k;
        int ret;
        k = EVP_PKEY_new();
        if (!k)
            return 0;
        EVP_PKEY_set1_EC_KEY(k, x);
        ret = PEM_write_PrivateKey(fp, k, enc, kstr, klen, cb, u);
        EVP_PKEY_free(k);
        return ret;
    } else
        return PEM_ASN1_write((i2d_of_void *)i2d_ECPrivateKey,
                              PEM_STRING_ECPRIVATEKEY,
                              fp, x, enc, kstr, klen, cb, u);
}
#  endif

# else
    IMPLEMENT_PEM_write_cb(ECPrivateKey, EC_KEY, PEM_STRING_ECPRIVATEKEY,
                       ECPrivateKey)
# endif
IMPLEMENT_PEM_rw(EC_PUBKEY, EC_KEY, PEM_STRING_PUBLIC, EC_PUBKEY)
# ifndef OPENSSL_NO_FP_API
EC_KEY *PEM_read_ECPrivateKey(FILE *fp, EC_KEY **eckey, pem_password_cb *cb,
                              void *u)
{
    EVP_PKEY *pktmp;
    pktmp = PEM_read_PrivateKey(fp, NULL, cb, u);
    return pkey_get_eckey(pktmp, eckey); /* will free pktmp */
}

# endif

#endif

#ifndef OPENSSL_NO_DH

IMPLEMENT_PEM_write_const(DHparams, DH, PEM_STRING_DHPARAMS, DHparams)
    IMPLEMENT_PEM_write_const(DHxparams, DH, PEM_STRING_DHXPARAMS, DHxparams)
#endif
IMPLEMENT_PEM_rw(PUBKEY, EVP_PKEY, PEM_STRING_PUBLIC, PUBKEY)
