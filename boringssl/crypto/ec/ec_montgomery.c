/* Originally written by Bodo Moeller and Nils Larsch for the OpenSSL project.
 * ====================================================================
 * Copyright (c) 1998-2005 The OpenSSL Project.  All rights reserved.
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
/* ====================================================================
 * Copyright 2002 Sun Microsystems, Inc. ALL RIGHTS RESERVED.
 *
 * Portions of the attached software ("Contribution") are developed by
 * SUN MICROSYSTEMS, INC., and are contributed to the OpenSSL project.
 *
 * The Contribution is licensed pursuant to the OpenSSL open source
 * license provided above.
 *
 * The elliptic curve binary polynomial software is originally written by
 * Sheueling Chang Shantz and Douglas Stebila of Sun Microsystems
 * Laboratories. */

#include <openssl/ec.h>

#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/mem.h>

#include "internal.h"


int ec_GFp_mont_group_init(EC_GROUP *group) {
  int ok;

  ok = ec_GFp_simple_group_init(group);
  group->mont = NULL;
  return ok;
}

void ec_GFp_mont_group_finish(EC_GROUP *group) {
  BN_MONT_CTX_free(group->mont);
  group->mont = NULL;
  ec_GFp_simple_group_finish(group);
}

int ec_GFp_mont_group_copy(EC_GROUP *dest, const EC_GROUP *src) {
  BN_MONT_CTX_free(dest->mont);
  dest->mont = NULL;

  if (!ec_GFp_simple_group_copy(dest, src)) {
    return 0;
  }

  if (src->mont != NULL) {
    dest->mont = BN_MONT_CTX_new();
    if (dest->mont == NULL) {
      return 0;
    }
    if (!BN_MONT_CTX_copy(dest->mont, src->mont)) {
      goto err;
    }
  }

  return 1;

err:
  BN_MONT_CTX_free(dest->mont);
  dest->mont = NULL;
  return 0;
}

int ec_GFp_mont_group_set_curve(EC_GROUP *group, const BIGNUM *p,
                                const BIGNUM *a, const BIGNUM *b, BN_CTX *ctx) {
  BN_CTX *new_ctx = NULL;
  BN_MONT_CTX *mont = NULL;
  int ret = 0;

  BN_MONT_CTX_free(group->mont);
  group->mont = NULL;

  if (ctx == NULL) {
    ctx = new_ctx = BN_CTX_new();
    if (ctx == NULL) {
      return 0;
    }
  }

  mont = BN_MONT_CTX_new();
  if (mont == NULL) {
    goto err;
  }
  if (!BN_MONT_CTX_set(mont, p, ctx)) {
    OPENSSL_PUT_ERROR(EC, ERR_R_BN_LIB);
    goto err;
  }

  group->mont = mont;
  mont = NULL;

  ret = ec_GFp_simple_group_set_curve(group, p, a, b, ctx);

  if (!ret) {
    BN_MONT_CTX_free(group->mont);
    group->mont = NULL;
  }

err:
  BN_CTX_free(new_ctx);
  BN_MONT_CTX_free(mont);
  return ret;
}

int ec_GFp_mont_field_mul(const EC_GROUP *group, BIGNUM *r, const BIGNUM *a,
                          const BIGNUM *b, BN_CTX *ctx) {
  if (group->mont == NULL) {
    OPENSSL_PUT_ERROR(EC, EC_R_NOT_INITIALIZED);
    return 0;
  }

  return BN_mod_mul_montgomery(r, a, b, group->mont, ctx);
}

int ec_GFp_mont_field_sqr(const EC_GROUP *group, BIGNUM *r, const BIGNUM *a,
                          BN_CTX *ctx) {
  if (group->mont == NULL) {
    OPENSSL_PUT_ERROR(EC, EC_R_NOT_INITIALIZED);
    return 0;
  }

  return BN_mod_mul_montgomery(r, a, a, group->mont, ctx);
}

int ec_GFp_mont_field_encode(const EC_GROUP *group, BIGNUM *r, const BIGNUM *a,
                             BN_CTX *ctx) {
  if (group->mont == NULL) {
    OPENSSL_PUT_ERROR(EC, EC_R_NOT_INITIALIZED);
    return 0;
  }

  return BN_to_montgomery(r, a, group->mont, ctx);
}

int ec_GFp_mont_field_decode(const EC_GROUP *group, BIGNUM *r, const BIGNUM *a,
                             BN_CTX *ctx) {
  if (group->mont == NULL) {
    OPENSSL_PUT_ERROR(EC, EC_R_NOT_INITIALIZED);
    return 0;
  }

  return BN_from_montgomery(r, a, group->mont, ctx);
}

static int ec_GFp_mont_check_pub_key_order(const EC_GROUP *group,
                                           const EC_POINT* pub_key,
                                           BN_CTX *ctx) {
  EC_POINT *point = EC_POINT_new(group);
  int ret = 0;

  if (point == NULL ||
      !ec_wNAF_mul(group, point, NULL, pub_key, EC_GROUP_get0_order(group),
                   ctx) ||
      !EC_POINT_is_at_infinity(group, point)) {
    goto err;
  }

  ret = 1;

err:
  EC_POINT_free(point);
  return ret;
}

static int ec_GFp_mont_point_get_affine_coordinates(const EC_GROUP *group,
                                                    const EC_POINT *point,
                                                    BIGNUM *x, BIGNUM *y,
                                                    BN_CTX *ctx) {
  BN_CTX *new_ctx = NULL;
  BIGNUM *Z, *Z_1, *Z_2, *Z_3;
  int ret = 0;

  if (EC_POINT_is_at_infinity(group, point)) {
    OPENSSL_PUT_ERROR(EC, EC_R_POINT_AT_INFINITY);
    return 0;
  }

  if (ctx == NULL) {
    ctx = new_ctx = BN_CTX_new();
    if (ctx == NULL) {
      return 0;
    }
  }

  BN_CTX_start(ctx);
  Z = BN_CTX_get(ctx);
  Z_1 = BN_CTX_get(ctx);
  Z_2 = BN_CTX_get(ctx);
  Z_3 = BN_CTX_get(ctx);
  if (Z == NULL || Z_1 == NULL || Z_2 == NULL || Z_3 == NULL) {
    goto err;
  }

  /* transform  (X, Y, Z)  into  (x, y) := (X/Z^2, Y/Z^3) */

  if (!group->meth->field_decode(group, Z, &point->Z, ctx)) {
    goto err;
  }

  if (BN_is_one(Z)) {
    if (x != NULL && !group->meth->field_decode(group, x, &point->X, ctx)) {
      goto err;
    }
    if (y != NULL && !group->meth->field_decode(group, y, &point->Y, ctx)) {
      goto err;
    }
  } else {
    if (!BN_mod_inverse(Z_1, Z, &group->field, ctx)) {
      OPENSSL_PUT_ERROR(EC, ERR_R_BN_LIB);
      goto err;
    }

    if (!BN_mod_sqr(Z_2, Z_1, &group->field, ctx)) {
      goto err;
    }

    /* in the Montgomery case, field_mul will cancel out Montgomery factor in
     * X: */
    if (x != NULL && !group->meth->field_mul(group, x, &point->X, Z_2, ctx)) {
      goto err;
    }

    if (y != NULL) {
      if (!BN_mod_mul(Z_3, Z_2, Z_1, &group->field, ctx)) {
        goto err;
      }

      /* in the Montgomery case, field_mul will cancel out Montgomery factor in
       * Y: */
      if (!group->meth->field_mul(group, y, &point->Y, Z_3, ctx)) {
        goto err;
      }
    }
  }

  ret = 1;

err:
  BN_CTX_end(ctx);
  BN_CTX_free(new_ctx);
  return ret;
}

const EC_METHOD *EC_GFp_mont_method(void) {
  static const EC_METHOD ret = {
    ec_GFp_mont_group_init,
    ec_GFp_mont_group_finish,
    ec_GFp_mont_group_copy,
    ec_GFp_mont_group_set_curve,
    ec_GFp_mont_point_get_affine_coordinates,
    ec_wNAF_mul /* XXX: Not constant time. */,
    ec_GFp_mont_check_pub_key_order,
    ec_GFp_mont_field_mul,
    ec_GFp_mont_field_sqr,
    ec_GFp_mont_field_encode,
    ec_GFp_mont_field_decode,
  };

  return &ret;
}
