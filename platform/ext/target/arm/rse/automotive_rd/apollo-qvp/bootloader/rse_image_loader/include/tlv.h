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
#include "bootutil/image.h"

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

int rse_find_tlv_by_type(const struct image_header *hdr,
                         const uint8_t *img_base,
                         bool prot,
                         uint16_t tlv_type,
                         uint32_t *tlv_off,
                         uint16_t *tlv_len);

#endif
