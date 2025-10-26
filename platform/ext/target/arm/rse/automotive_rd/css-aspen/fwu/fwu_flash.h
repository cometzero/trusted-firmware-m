/*
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __FWU_FLASH_H_
#define __FWU_FLASH_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "device_definition.h"
#include "Driver_Flash.h"
#include "fip_parser/uuid.h"
#include "host_ap_memory_map.h"
#include "host_atu_base_address.h"
#include "psa/update.h"

#include <stdint.h>

#define FWU_STORE_INITIALIZED       (1U)
#define FWU_STORE_UNINITIALIZED     (0U)

/* Represents both PSA_FWU_WRITING and PSA_FWU_CANDIDATE states as combined state for Shim layer */
#define PSA_FWU_WRITING_CANDIDATE   (8u)

/**
 * Describes the location and identity of a firmware image in flash.
 *
 * This structure is used to define per-image metadata for firmware
 * update operations, typically per-bank in A/B schemes.
 */
struct fwu_image_location {
    /* Size of the firmware image partition in bytes. */
    uint32_t partition_size;

    /* Offset (from bank base) where the image is stored in flash. */
    uint32_t partition_offset[NR_OF_FW_BANKS];

    /* Globally unique identifier (UUID) for the image payload. */
    struct efi_guid image_guid;

    /* PSA FWU component index corresponding to this image. */
    psa_fwu_component_t component;

    /* The flash memory driver instance for RSE flash, AP flash */
    ARM_DRIVER_FLASH *flash;
};

const struct fwu_image_location* fwu_get_image_location(psa_fwu_component_t component);

psa_status_t fwu_flash_read(ARM_DRIVER_FLASH *flash, uint32_t partition_offset,
                            void *data, uint32_t size);
psa_status_t fwu_flash_write(ARM_DRIVER_FLASH *flash, uint32_t partition_offset,
                             const void *data, uint32_t size);
psa_status_t fwu_flash_erase(ARM_DRIVER_FLASH *flash, uint32_t partition_offset,
                             uint32_t size);

/* Configure RSE ATU to access AP Secure Flash for FWU metadata */
#define INIT_ATU_REGION_FOR_AP_FLASH()                                      \
        do {                                                                \
            if (atu_rse_initialize_region(&ATU_DEV_S, HOST_AP_FLASH_ATU_ID, \
                    HOST_AP_FLASH_BASE, HOST_AP_FLASH_PHY_BASE,             \
                    HOST_AP_FLASH_SIZE) != ATU_ERR_NONE) {                  \
                return PSA_ERROR_GENERIC_ERROR;                             \
            }                                                               \
        } while (0)

/* Close the RSE ATU to access AP Secure Flash */
#define DEINIT_ATU_REGION_FOR_AP_FLASH()                                        \
        do {                                                                    \
            if (atu_rse_uninitialize_region(&ATU_DEV_S, HOST_AP_FLASH_ATU_ID)   \
                    != ATU_ERR_NONE) {                                          \
                return PSA_ERROR_GENERIC_ERROR;                                 \
            }                                                                   \
        } while (0)

#ifdef __cplusplus
}
#endif

#endif /* __FWU_FLASH_H_ */
