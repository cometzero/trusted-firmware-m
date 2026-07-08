/*
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __FWU_METADATA_V2_H_
#define __FWU_METADATA_V2_H_

#include "psa/error.h"
#include "psa/update.h"
#include "fip_parser/uuid.h"

#include <stdint.h>

/*
 * Metadata version 2 data structures defined by PSA_FW update specification
 * at https://developer.arm.com/documentation/den0118/latest/
 */
#define FWU_METADATA_VERSION		(2U)

#define FWU_BANK_STATE_ACCEPTED     (0xFCU)
#define FWU_BANK_STATE_VALID        (0xFEU)
#define FWU_BANK_STATE_INVALID      (0xFFU)

#define FWU_IMAGE_NOT_ACCEPTED      (0)
#define FWU_IMAGE_ACCEPTED          (1)

/* Properties of image in a bank */
struct fwu_image_properties {
    /* The UUID of the image in this bank */
    struct efi_guid img_uuid;

    /* [0]: bit describing the image acceptance status –
     * status - 1 means the image is accepted[31:1]: MBZ */
    uint32_t accepted;

    /* NOTE: using the reserved field */
    /* image version */
    uint32_t version;
} __attribute__((packed));

/* Image entry information */
struct fwu_image_entry {
    /* The UUID identifying the image type */
    struct efi_guid img_type_uuid;

    /* The UUID of the storage volume where the image is located */
    struct efi_guid location_uuid;

    /* The Properties of images with img_type_uuid in the different FW banks */
    struct fwu_image_properties img_props[NR_OF_FW_BANKS];
} __attribute__((packed));

struct fwu_fw_store_descriptor {
    /* The number of firmware banks in the Firmware Store */
    uint8_t num_banks;

    /* Reserved */
    uint8_t reserved;

    /* The number of images per bank. This should be the number of entries in
     * the img_entry array */
    uint16_t num_images;

    /* The size of image_entry(all banks) in bytes */
    uint16_t img_entry_size;

    /* The size of image bank info structure in bytes */
    uint16_t bank_info_entry_size;

    /* Array of fwu_image_entry structs */
    struct fwu_image_entry img_entry[FWU_COMPONENT_NUMBER];
} __attribute__((packed));

struct fwu_metadata_v2 {
    /* The metadata CRC value */
    uint32_t crc_32;

    /* The metadata version */
    uint32_t version;

    /* The bank index with which device boots */
    uint32_t active_index;

    /* The previous bank index with which device booted successfully */
    uint32_t previous_active_index;

    /* The size of the entire metadata in bytes */
    uint32_t metadata_size;

    /* The offset of the image descriptor structure */
    uint16_t desc_offset;

    /* Reserved */
    uint16_t reserved1;

    /* The state of each bank
     * Each bank_state entry can take one of the following values:
     * • 0xFF: invalid – One or more images in the bank are corrupted or were partially overwritten.
     * • 0xFE: valid – The bank contains a valid set of images, but some images are in an unaccepted state.
     * • 0xFC: accepted – all of the images in the bank are valid and have been accepted.
     */
    uint8_t bank_state[4];

    /* Reserved */
    uint32_t reserved2;

    /* FWU metadata v2 store descriptor */
    struct fwu_fw_store_descriptor fw_desc;
} __attribute__((packed));

psa_status_t fwu_metadata_init(void);
psa_status_t fwu_metadata_deinit(void);
psa_status_t fwu_metadata_write(struct fwu_metadata_v2 *metadata);
psa_status_t fwu_metadata_read(struct fwu_metadata_v2 *metadata);

#endif /* __FWU_METADATA_V2_H_ */
