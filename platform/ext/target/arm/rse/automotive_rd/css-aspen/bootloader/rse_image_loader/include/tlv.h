/*                                                                                                                                                                                                       
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 */

#ifndef TLV_H
#define TLV_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#define IMAGE_TLV_ENC_KW           0x31
#define IMAGE_TLV_INFO_MAGIC       0x6907
#define IMAGE_TLV_PROT_INFO_MAGIC  0x6908

enum rse_tlv_status {
    RSE_TLV_FOUND            = 0,
    RSE_TLV_NOT_FOUND        = 1,

    RSE_TLV_ERR_INVALID_ARG      = -1,
    RSE_TLV_ERR_PROT_SIZE_MISMATCH = -2,
    RSE_TLV_ERR_PROT_INFO_MISSING  = -3,
    RSE_TLV_ERR_BAD_MAGIC        = -4,
    RSE_TLV_ERR_TRUNC_HDR        = -5,
    RSE_TLV_ERR_TRUNC_PAYLOAD    = -6
};

struct image_header {
    uint32_t ih_magic;
    uint32_t ih_load_addr;
    uint16_t ih_hdr_size;
    uint16_t ih_protect_tlv_size;
    uint32_t ih_img_size;
};

struct image_tlv_info {
    uint16_t it_magic;
    uint16_t it_tlv_tot;
};

struct image_tlv {
    uint8_t  it_type;
    uint8_t  _pad;
    uint16_t it_len;
};

int rse_find_tlv(const struct image_header *hdr,
         const uint8_t *img_base,
         bool prot,
         uint32_t *enc_kw_off,
         uint16_t *enc_kw_len);
#endif
