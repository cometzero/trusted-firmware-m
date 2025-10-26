/*
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "device_definition.h"
#include "flash_layout.h"
#include "fwu_flash.h"
#include "host_ap_memory_map.h"
#include "host_atu_base_address.h"
#include "psa/update.h"

#include <assert.h>

#define BANK_INFO_SIZE    (sizeof(bank_info)/sizeof(bank_info[0]))

/*
 * Flash driver for accessing RSE-local flash.
 * This device is used for storing private FWU metadata.
 */
extern ARM_DRIVER_FLASH FLASH_DEV_NAME;

/*
 * Flash driver for accessing AP-hosted flash via ATU.
 * This device stores the formal FWU metadata structure
 */
extern ARM_DRIVER_FLASH AP_FLASH_DEV_NAME;

static const struct fwu_image_location bank_info[] = {
    {
        .partition_size = FLASH_BL2_PARTITION_SIZE,
        .partition_offset = {FLASH_AREA_0_OFFSET, FLASH_AREA_1_OFFSET},
        .component = (psa_fwu_component_t)FWU_COMPONENT_INDEX_BL2,
        .flash = &FLASH_DEV_NAME,
    },
    {
        .partition_size = FLASH_S_PARTITION_SIZE,
        .partition_offset = {FLASH_AREA_2_OFFSET, FLASH_AREA_3_OFFSET},
        .component = (psa_fwu_component_t)FWU_COMPONENT_INDEX_RSE_RUNTIME,
        .flash = &FLASH_DEV_NAME,
    },
    {
        .partition_size = FLASH_SI_CL0_PARTITION_SIZE,
        .partition_offset = {FLASH_AREA_4_OFFSET, FLASH_AREA_5_OFFSET},
        .component = (psa_fwu_component_t)FWU_COMPONENT_INDEX_SI_CL0,
        .flash = &FLASH_DEV_NAME,
    },
    {
        .partition_size = AP_FLASH_FIP_SIZE,
        .partition_offset = {AP_FLASH_AREA_0_OFFSET, AP_FLASH_AREA_1_OFFSET},
        .component = (psa_fwu_component_t)FWU_COMPONENT_INDEX_AP_FIP_IMAGE,
        .flash = &AP_FLASH_DEV_NAME,
    },
};

const struct fwu_image_location* fwu_get_image_location(psa_fwu_component_t component)
{
    uint8_t idx;

    assert(component < (psa_fwu_component_t)FWU_COMPONENT_NUMBER);

    for (idx = 0; idx < BANK_INFO_SIZE; idx++) {
        if (bank_info[idx].component == component) {
            return &bank_info[idx];
        }
    }

    return NULL;
}

psa_status_t fwu_flash_read(ARM_DRIVER_FLASH *flash, uint32_t partition_offset,
                            void *data, uint32_t size)
{
    int ret;

    if (flash == NULL || data == NULL) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    if (flash == &AP_FLASH_DEV_NAME) {
        INIT_ATU_REGION_FOR_AP_FLASH();
    }

    ret = flash->ReadData(partition_offset, data, size);

    if (flash == &AP_FLASH_DEV_NAME) {
        DEINIT_ATU_REGION_FOR_AP_FLASH();
    }

    if (ret < 0) {
        return PSA_ERROR_STORAGE_FAILURE;
    }

    return PSA_SUCCESS;
}

psa_status_t fwu_flash_write(ARM_DRIVER_FLASH *flash, uint32_t partition_offset, const void *data,
                             uint32_t size)
{
    int ret;

    if (!size || (flash == NULL) || (data == NULL)) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    if (flash == &AP_FLASH_DEV_NAME) {
        INIT_ATU_REGION_FOR_AP_FLASH();
    }

    ret = flash->ProgramData(partition_offset, data, size);

    if (flash == &AP_FLASH_DEV_NAME) {
        DEINIT_ATU_REGION_FOR_AP_FLASH();
    }

    if ((ret < 0) || (ret != size)) {
        return PSA_ERROR_STORAGE_FAILURE;
    }

    return PSA_SUCCESS;
}

psa_status_t fwu_flash_erase(ARM_DRIVER_FLASH *flash, uint32_t partition_offset,
                             uint32_t size)
{
    const ARM_FLASH_INFO *flash_info;
    psa_status_t status = PSA_SUCCESS;

    if (!size || (flash == NULL)) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    if (flash == &AP_FLASH_DEV_NAME) {
        INIT_ATU_REGION_FOR_AP_FLASH();
    }

    flash_info = flash->GetInfo();
    for (uint32_t offset = partition_offset; offset < (partition_offset + size);
            offset += flash_info->sector_size) {
        if (flash->EraseSector(offset) != ARM_DRIVER_OK) {
            status = PSA_ERROR_STORAGE_FAILURE;
            break;
        }
    }

    if (flash == &AP_FLASH_DEV_NAME) {
        DEINIT_ATU_REGION_FOR_AP_FLASH();
    }

    return status;
}
