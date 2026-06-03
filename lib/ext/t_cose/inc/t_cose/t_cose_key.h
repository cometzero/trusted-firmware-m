/*
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TFM_T_COSE_KEY_COMPAT_H
#define TFM_T_COSE_KEY_COMPAT_H

#include_next <t_cose/t_cose_key.h>

#ifdef __cplusplus
extern "C" {
#endif

enum t_cose_err_t
t_cose_key_encode(struct t_cose_key      key,
                  struct q_useful_buf    key_buf,
                  struct q_useful_buf_c *cbor_encoded);

#ifdef __cplusplus
}
#endif

#endif /* TFM_T_COSE_KEY_COMPAT_H */
