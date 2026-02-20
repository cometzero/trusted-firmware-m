 /*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 */

#include "tlv.h"

/*
 * Find the ENCKW (encryption key) TLV and return its payload offset.
 *
 * TLV Region Layout
 *
 * The TLV region starts at:
 *     off_base = hdr->ih_hdr_size + hdr->ih_img_size
 *
 *   +-------------------------------+
 *   | Protected TLV Info Header     |  (magic: IMAGE_TLV_PROT_INFO_MAGIC)
 *   |  - it_tlv_tot = protected_sz  |
 *   +-------------------------------+
 *   | Protected TLVs                |
 *   |   [ type | len | value ]      |
 *   |   [ type | len | value ]      |
 *   |   ...                         |
 *   +-------------------------------+
 *   | Non-Protected TLV Info Header |  (magic: IMAGE_TLV_INFO_MAGIC)
 *   |  - it_tlv_tot = nonprot_sz    |
 *   +-------------------------------+
 *   | Non-Protected TLVs            |
 *   |   [ type | len | value ]      |
 *   |   [ type | len | value ]      |
 *   |   [ ENCKW TLV ]               |
 *   |   ...                         |
 *   +-------------------------------+
 *
 * @param hdr         Image_header of the slot's image
 * @param img_base    Base address of the image in RAM (offset 0 of image).
 * @param prot        true: search only protected TLVs, false: search only non-protected TLVs
 * @param enc_kw_off  [out] offset of ENCKW TLV payload in flash
 * @param enc_kw_len  [out, optional] length of ENCKW TLV payload
 *
 * @returns 0  if ENCKW TLV was found
 *          1  if ENCKW TLV was not found
 *         <0  on errors
 */
int rse_find_tlv(const struct image_header *hdr,
        const uint8_t *img_base,
        bool prot,
        uint32_t *enc_kw_off,
        uint16_t *enc_kw_len)
{
    struct image_tlv_info info;
    struct image_tlv tlv;
    uint32_t off_base, prot_end, tlv_end, tlv_off;

    if (!hdr || !img_base || !enc_kw_off) {
        return RSE_TLV_ERR_INVALID_ARG;
    }

    off_base = hdr->ih_hdr_size + hdr->ih_img_size;

    /* Ensure the base read is within the loaded buffer is caller's responsibility */

    memcpy(&info, img_base + off_base, sizeof(info));

    if (info.it_magic == IMAGE_TLV_PROT_INFO_MAGIC) {
        if (hdr->ih_protect_tlv_size != info.it_tlv_tot) {
            return RSE_TLV_ERR_PROT_SIZE_MISMATCH;
        }
        memcpy(&info, img_base + off_base + info.it_tlv_tot, sizeof(info));
    } else if (hdr->ih_protect_tlv_size != 0) {
        return RSE_TLV_ERR_PROT_INFO_MISSING;
    }

    if (info.it_magic != IMAGE_TLV_INFO_MAGIC) {
        return RSE_TLV_ERR_BAD_MAGIC;
    }

    prot_end = off_base + hdr->ih_protect_tlv_size;
    tlv_end  = off_base + hdr->ih_protect_tlv_size + info.it_tlv_tot;

    tlv_off = off_base + sizeof(info);

    while (tlv_off < tlv_end) {
        if (hdr->ih_protect_tlv_size > 0 && tlv_off == prot_end) {
            /* Ensure the extra image_tlv_info header fits before skipping it. */
            if (tlv_end - tlv_off < (uint32_t)sizeof(struct image_tlv_info)) {
                return RSE_TLV_ERR_TRUNC_HDR;
            }

            tlv_off += (uint32_t)sizeof(struct image_tlv_info);

            /* If we've reached or passed the end, there are no more TLVs. */
            if (tlv_off >= tlv_end) {
                return RSE_TLV_NOT_FOUND;
            }
        }

        /* Ensure there is room for a TLV header */
        if (tlv_end - tlv_off < (uint32_t)sizeof(tlv)) {
            return RSE_TLV_ERR_TRUNC_HDR;
        }

        memcpy(&tlv, img_base + tlv_off, sizeof(tlv));

        if (prot && tlv_off >= prot_end) {
            return RSE_TLV_NOT_FOUND;
        }

        if (tlv.it_type == IMAGE_TLV_ENC_KW) {
            *enc_kw_off = tlv_off + sizeof(tlv);
            if (enc_kw_len) {
                *enc_kw_len = tlv.it_len;
            }
            return RSE_TLV_FOUND;
        }

        /* Ensure TLV payload fits before advancing past it. */
        if (tlv.it_len > (tlv_end - (tlv_off + (uint32_t)sizeof(tlv)))) {
            return RSE_TLV_ERR_TRUNC_PAYLOAD;
        }

        tlv_off += (uint32_t)sizeof(tlv) + tlv.it_len;
    }

    return RSE_TLV_NOT_FOUND;
}
