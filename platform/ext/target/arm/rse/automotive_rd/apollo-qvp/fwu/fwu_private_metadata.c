/*
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */


#include "Driver_Flash.h"
#include "flash_layout.h"
#include "fwu_flash.h"
#include "fwu_private_metadata.h"
#include "psa/error.h"
#include "psa/update.h"
#include "tfm_log.h"

#include <stdint.h>

/*
 * Flash driver for accessing RSE-local flash.
 * This device is used for storing private FWU metadata.
 */
extern ARM_DRIVER_FLASH FLASH_DEV_NAME;

/*
 * RSE flash will be uninitialized in state.
 */
static uint8_t rse_flash_state = FWU_STORE_UNINITIALIZED;

psa_status_t fwu_private_metadata_read(struct fwu_private_metadata *metadata)
{
    int ret;

    if (metadata == NULL) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    ret = FLASH_DEV_NAME.ReadData(FWU_PRIVATE_METADATA_REPLICA_1_OFFSET,
                                  metadata, sizeof(*metadata));
    if (ret < 0) {
        ERROR("FWU: unable to read private metadata\n");
        return PSA_ERROR_STORAGE_FAILURE;
    }
    if (ret != sizeof(*metadata)) {
        return PSA_ERROR_INSUFFICIENT_DATA;
    }
    return PSA_SUCCESS;
}

psa_status_t fwu_private_metadata_write(const struct fwu_private_metadata *metadata)
{
    int ret;

    if (metadata == NULL) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    ret = FLASH_DEV_NAME.EraseSector(FWU_PRIVATE_METADATA_REPLICA_1_OFFSET);
    if (ret != ARM_DRIVER_OK) {
        return PSA_ERROR_STORAGE_FAILURE;
    }

    ret = FLASH_DEV_NAME.ProgramData(FWU_PRIVATE_METADATA_REPLICA_1_OFFSET,
                                     metadata, sizeof(*metadata));
    if (ret < 0) {
        ERROR("FWU: unable to write private metadata\n");
        return PSA_ERROR_STORAGE_FAILURE;
    }
    if (ret != sizeof(*metadata)) {
        return PSA_ERROR_INSUFFICIENT_STORAGE;
    }

    return PSA_SUCCESS;
}

psa_status_t fwu_private_metadata_init(void)
{
    if (FLASH_DEV_NAME.Initialize(NULL) != ARM_DRIVER_OK) {
        rse_flash_state = FWU_STORE_UNINITIALIZED;
        return PSA_ERROR_STORAGE_FAILURE;
    }

    rse_flash_state = FWU_STORE_INITIALIZED;
    return PSA_SUCCESS;
}

psa_status_t fwu_private_metadata_deinit(void)
{
    if (FLASH_DEV_NAME.Uninitialize() != ARM_DRIVER_OK) {
        return PSA_ERROR_STORAGE_FAILURE;
    }

    rse_flash_state = FWU_STORE_UNINITIALIZED;
    return PSA_SUCCESS;
}
