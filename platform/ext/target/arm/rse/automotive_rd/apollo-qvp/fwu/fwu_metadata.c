/*
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "device_definition.h"
#include "Driver_Flash.h"
#include "flash_layout.h"
#include "fwu_flash.h"
#include "fwu_metadata_v2.h"
#include "host_ap_memory_map.h"
#include "host_atu_base_address.h"
#include "psa/error.h"
#include "psa/update.h"
#include "soft_crc.h"
#include "tfm_log.h"

#include <stdint.h>

/*
 * Flash driver for accessing AP-hosted flash via ATU.
 * This device stores the formal FWU metadata structure
 */
extern ARM_DRIVER_FLASH AP_FLASH_DEV_NAME;

/*
 * AP flash will be uninitialized in state.
 */
static uint8_t ap_flash_state = FWU_STORE_UNINITIALIZED;

psa_status_t fwu_metadata_read(struct fwu_metadata_v2 *metadata)
{
    uint32_t calc_crc32;
    int ret;

    if (metadata == NULL) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    if (ap_flash_state != FWU_STORE_INITIALIZED) {
        return PSA_ERROR_BAD_STATE;
    }

    INIT_ATU_REGION_FOR_AP_FLASH();

    ret = AP_FLASH_DEV_NAME.ReadData(FWU_METADATA_REPLICA_1_OFFSET, metadata,
                                     sizeof(*metadata));

    DEINIT_ATU_REGION_FOR_AP_FLASH();

    if (ret < 0) {
        return PSA_ERROR_STORAGE_FAILURE;
    }
    if (ret != sizeof(*metadata)) {
        return PSA_ERROR_INSUFFICIENT_DATA;
    }

    calc_crc32 = crc32(&metadata->version,
                       (sizeof(*metadata) - sizeof(metadata->crc_32)));
    if (calc_crc32 != metadata->crc_32) {
        ERROR("FWU: metadata crc expected 0x%x, got 0x%x\n", metadata->crc_32,
                                                             calc_crc32);
        return PSA_ERROR_GENERIC_ERROR;
    }

    return PSA_SUCCESS;
}

psa_status_t fwu_metadata_write(struct fwu_metadata_v2 *metadata)
{
    int ret;

    if (metadata == NULL) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    if (ap_flash_state != FWU_STORE_INITIALIZED) {
        return PSA_ERROR_BAD_STATE;
    }

    metadata->crc_32 = crc32(&metadata->version,
                             (sizeof(*metadata) - sizeof(metadata->crc_32)));

    INIT_ATU_REGION_FOR_AP_FLASH();

    ret = AP_FLASH_DEV_NAME.EraseSector(FWU_METADATA_REPLICA_1_OFFSET);
    if (ret != ARM_DRIVER_OK) {
        DEINIT_ATU_REGION_FOR_AP_FLASH();
        return PSA_ERROR_STORAGE_FAILURE;
    }

    ret = AP_FLASH_DEV_NAME.ProgramData(FWU_METADATA_REPLICA_1_OFFSET, metadata,
            sizeof(*metadata));

    DEINIT_ATU_REGION_FOR_AP_FLASH();

    if (ret < 0) {
        return PSA_ERROR_STORAGE_FAILURE;
    }
    if (ret != sizeof(*metadata)) {
        return PSA_ERROR_INSUFFICIENT_DATA;
    }

    return PSA_SUCCESS;
}

psa_status_t fwu_metadata_init(void)
{
    if (AP_FLASH_DEV_NAME.Initialize(NULL) != ARM_DRIVER_OK) {
        ap_flash_state = FWU_STORE_UNINITIALIZED;
        return PSA_ERROR_STORAGE_FAILURE;
    }

    ap_flash_state = FWU_STORE_INITIALIZED;
    return PSA_SUCCESS;
}

psa_status_t fwu_metadata_deinit(void)
{
    if (AP_FLASH_DEV_NAME.Uninitialize() != ARM_DRIVER_OK) {
        return PSA_ERROR_STORAGE_FAILURE;
    }

    ap_flash_state = FWU_STORE_UNINITIALIZED;
    return PSA_SUCCESS;
}
