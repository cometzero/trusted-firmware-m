/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 */

#include <string.h>
#include <stdint.h>
#include "Driver_Flash.h"
#include "RTE_Device.h"
#include "platform_base_address.h"
#include "host_base_address.h"

#ifndef ARG_UNUSED
#define ARG_UNUSED(arg)  ((void)arg)
#endif

/* Driver version */
#define ARM_FLASH_DRV_VERSION      ARM_DRIVER_VERSION_MAJOR_MINOR(1, 1)
#define ARM_FLASH_DRV_ERASE_VALUE  0xFF

/**
 * Data width values for ARM_FLASH_CAPABILITIES::data_width
 * \ref ARM_FLASH_CAPABILITIES
 */
 enum {
    DATA_WIDTH_8BIT   = 0u,
    DATA_WIDTH_16BIT,
    DATA_WIDTH_32BIT,
    DATA_WIDTH_ENUM_SIZE
};

static const uint32_t data_width_byte[DATA_WIDTH_ENUM_SIZE] = {
    sizeof(uint8_t),
    sizeof(uint16_t),
    sizeof(uint32_t),
};

/*
 * ARM FLASH device structure
 *
 * This driver just emulates a flash interface and behaviour on top of the SRAM
 * memory.
 */
struct arm_flash_dev_t {
    const uint32_t memory_base;   /*!< FLASH memory base address */
    ARM_FLASH_INFO *data;         /*!< FLASH data */
};

/* Flash Status */
static ARM_FLASH_STATUS FlashStatus = {
    0, /* Flash busy flag. */
    0, /* Read/Program/Erase error flag */
    0  /* Reserved */
};

/* Driver Version */
static const ARM_DRIVER_VERSION DriverVersion = {
    ARM_FLASH_API_VERSION,
    ARM_FLASH_DRV_VERSION
};

/* Driver Capabilities */
static const ARM_FLASH_CAPABILITIES DriverCapabilities = {
    0, /* event_ready */
    0, /* data_width = 0:8-bit, 1:16-bit, 2:32-bit */
    1  /* erase_chip */
};

static int32_t is_range_valid(struct arm_flash_dev_t *flash_dev,
                              uint32_t offset)
{
    uint32_t flash_limit = 0;
    int32_t rc = 0;

    if ((flash_dev->data->sector_count == 0) ||
        (flash_dev->data->sector_size == 0)) {
        return -1;
    }

    flash_limit = (flash_dev->data->sector_count * flash_dev->data->sector_size)
                   - 1;

    if (offset > flash_limit) {
        rc = -1;
    }
    return rc;
}

static int32_t is_write_aligned(struct arm_flash_dev_t *flash_dev,
                                uint32_t param)
{
    int32_t rc = 0;

    if ((param % flash_dev->data->program_unit) != 0) {
        rc = -1;
    }
    return rc;
}

static int32_t is_sector_aligned(struct arm_flash_dev_t *flash_dev,
                                 uint32_t offset)
{
    int32_t rc = 0;

    if ((offset % flash_dev->data->sector_size) != 0) {
        rc = -1;
    }
    return rc;
}

static int32_t is_flash_ready_to_write(const uint8_t *start_addr, uint32_t cnt)
{
    int32_t rc = 0;
    uint32_t i;

    for (i = 0; i < cnt; i++) {
        if(start_addr[i] != ARM_FLASH_DRV_ERASE_VALUE) {
            rc = -1;
            break;
        }
    }

    return rc;
}

static ARM_FLASH_INFO ARM_FLASH0_DEV_DATA = {
    .sector_info    = NULL,     /* Uniform sector layout */
    .sector_count   = BOOT_FLASH_SIZE / 0x1000,
    .sector_size    = 0x1000,
    .page_size      = 256U,
    .program_unit   = 1U,
    .erased_value   = ARM_FLASH_DRV_ERASE_VALUE
};

static struct arm_flash_dev_t ARM_FLASH0_DEV = {
    .memory_base = BOOT_FLASH_BASE_S,
    .data        = &ARM_FLASH0_DEV_DATA
};

struct arm_flash_dev_t *FLASH0_DEV = &ARM_FLASH0_DEV;

/*
 * Functions
 */

static ARM_DRIVER_VERSION ARM_Flash0_GetVersion(void)
{
    return DriverVersion;
}

static ARM_FLASH_CAPABILITIES ARM_Flash0_GetCapabilities(void)
{
    return DriverCapabilities;
}

static int32_t ARM_Flash0_Initialize(ARM_Flash_SignalEvent_t cb_event)
{
    ARG_UNUSED(cb_event);

    if (DriverCapabilities.data_width >= DATA_WIDTH_ENUM_SIZE) {
        return ARM_DRIVER_ERROR;
    }

    /* Nothing to be done */
    return ARM_DRIVER_OK;
}

static int32_t ARM_Flash0_Uninitialize(void)
{
    /* Nothing to be done */
    return ARM_DRIVER_OK;
}

static int32_t ARM_Flash0_PowerControl(ARM_POWER_STATE state)
{
    switch (state) {
    case ARM_POWER_FULL:
        /* Nothing to be done */
        return ARM_DRIVER_OK;
        break;

    case ARM_POWER_OFF:
    case ARM_POWER_LOW:
    default:
        return ARM_DRIVER_ERROR_UNSUPPORTED;
    }
}

static int32_t ARM_Flash0_ReadData(uint32_t addr, void *data, uint32_t cnt)
{
    uint32_t start_addr = FLASH0_DEV->memory_base + addr;
    uint32_t byte_cnt = 0;
    int32_t rc = 0;

    /* Conversion between data items and bytes */
    byte_cnt = cnt * data_width_byte[DriverCapabilities.data_width];

    /* Check flash memory boundaries */
    rc = is_range_valid(FLASH0_DEV, addr + byte_cnt);
    if (rc != 0) {
        return ARM_DRIVER_ERROR_PARAMETER;
    }

    /* Flash interface just emulated over SRAM, use memcpy */
    memcpy(data, (void *)start_addr, byte_cnt);

    return cnt;
}

static int32_t ARM_Flash0_ProgramData(uint32_t addr, const void *data,
                                      uint32_t cnt)
{
    uint32_t start_addr = FLASH0_DEV->memory_base + addr;
    uint32_t byte_cnt = 0;
    int32_t rc = 0;

    /* Conversion between data items and bytes */
    byte_cnt = cnt * data_width_byte[DriverCapabilities.data_width];

    /* Check flash memory boundaries and alignment with minimal write size */
    rc  = is_range_valid(FLASH0_DEV, addr + byte_cnt);
    rc |= is_write_aligned(FLASH0_DEV, addr);
    rc |= is_write_aligned(FLASH0_DEV, byte_cnt);
    if (rc != 0) {
        return ARM_DRIVER_ERROR_PARAMETER;
    }

    /* Check if the flash area to write the data was erased previously */
    rc = is_flash_ready_to_write((const uint8_t*)start_addr, byte_cnt);
    if (rc != 0) {
        return ARM_DRIVER_ERROR;
    }

    /* Flash interface just emulated over SRAM, use memcpy */
    memcpy((void *)start_addr, data, byte_cnt);

    return cnt;
}

static int32_t ARM_Flash0_EraseSector(uint32_t addr)
{
    uint32_t start_addr = FLASH0_DEV->memory_base + addr;
    uint32_t rc = 0;

    rc  = is_range_valid(FLASH0_DEV, addr);
    rc |= is_sector_aligned(FLASH0_DEV, addr);
    if (rc != 0) {
        return ARM_DRIVER_ERROR_PARAMETER;
    }

    /* Flash interface just emulated over SRAM, use memset */
    memset((void *)start_addr,
           FLASH0_DEV->data->erased_value,
           FLASH0_DEV->data->sector_size);
    return ARM_DRIVER_OK;
}

static int32_t ARM_Flash0_EraseChip(void)
{
    uint32_t i;
    uint32_t addr = FLASH0_DEV->memory_base;
    int32_t rc = ARM_DRIVER_ERROR_UNSUPPORTED;

    /* Check driver capability erase_chip bit */
    if (DriverCapabilities.erase_chip == 1) {
        for (i = 0; i < FLASH0_DEV->data->sector_count; i++) {
            /* Flash interface just emulated over SRAM, use memset */
            memset((void *)addr,
                   FLASH0_DEV->data->erased_value,
                   FLASH0_DEV->data->sector_size);

            addr += FLASH0_DEV->data->sector_size;
            rc = ARM_DRIVER_OK;
        }
    }
    return rc;
}

static ARM_FLASH_STATUS ARM_Flash0_GetStatus(void)
{
    return FlashStatus;
}

static ARM_FLASH_INFO * ARM_Flash0_GetInfo(void)
{
    return FLASH0_DEV->data;
}

ARM_DRIVER_FLASH Driver_FLASH0 = {
    ARM_Flash0_GetVersion,
    ARM_Flash0_GetCapabilities,
    ARM_Flash0_Initialize,
    ARM_Flash0_Uninitialize,
    ARM_Flash0_PowerControl,
    ARM_Flash0_ReadData,
    ARM_Flash0_ProgramData,
    ARM_Flash0_EraseSector,
    ARM_Flash0_EraseChip,
    ARM_Flash0_GetStatus,
    ARM_Flash0_GetInfo
};

static ARM_FLASH_INFO ARM_FLASH1_DEV_DATA = {
    .sector_info    = NULL,     /* Uniform sector layout */
    .sector_count   = AP_BOOT_FLASH_SIZE / 0x1000,
    .sector_size    = 0x1000,
    .page_size      = 256U,
    .program_unit   = 1U,
    .erased_value   = ARM_FLASH_DRV_ERASE_VALUE
};

static struct arm_flash_dev_t ARM_FLASH1_DEV = {
    .memory_base = HOST_AP_FLASH_BASE,
    .data        = &ARM_FLASH1_DEV_DATA
};

struct arm_flash_dev_t *FLASH1_DEV = &ARM_FLASH1_DEV;

/*
 * Functions
 */

static ARM_DRIVER_VERSION ARM_Flash1_GetVersion(void)
{
    return DriverVersion;
}

static ARM_FLASH_CAPABILITIES ARM_Flash1_GetCapabilities(void)
{
    return DriverCapabilities;
}

static int32_t ARM_Flash1_Initialize(ARM_Flash_SignalEvent_t cb_event)
{
    ARG_UNUSED(cb_event);

    if (DriverCapabilities.data_width >= DATA_WIDTH_ENUM_SIZE) {
        return ARM_DRIVER_ERROR;
    }

    /* Nothing to be done */
    return ARM_DRIVER_OK;
}

static int32_t ARM_Flash1_Uninitialize(void)
{
    /* Nothing to be done */
    return ARM_DRIVER_OK;
}

static int32_t ARM_Flash1_PowerControl(ARM_POWER_STATE state)
{
    switch (state) {
    case ARM_POWER_FULL:
        /* Nothing to be done */
        return ARM_DRIVER_OK;
        break;

    case ARM_POWER_OFF:
    case ARM_POWER_LOW:
    default:
        return ARM_DRIVER_ERROR_UNSUPPORTED;
    }
}

static int32_t ARM_Flash1_ReadData(uint32_t addr, void *data, uint32_t cnt)
{
    uint32_t start_addr = FLASH1_DEV->memory_base + addr;
    uint32_t byte_cnt = 0;
    int32_t rc = 0;

    /* Conversion between data items and bytes */
    byte_cnt = cnt * data_width_byte[DriverCapabilities.data_width];

    /* Check flash memory boundaries */
    rc = is_range_valid(FLASH1_DEV, addr + byte_cnt);
    if (rc != 0) {
        return ARM_DRIVER_ERROR_PARAMETER;
    }

    /* Flash interface just emulated over SRAM, use memcpy */
    memcpy(data, (void *)start_addr, byte_cnt);

    return cnt;
}

static int32_t ARM_Flash1_ProgramData(uint32_t addr, const void *data,
                                      uint32_t cnt)
{
    uint32_t start_addr = FLASH1_DEV->memory_base + addr;
    uint32_t byte_cnt = 0;
    int32_t rc = 0;

    /* Conversion between data items and bytes */
    byte_cnt = cnt * data_width_byte[DriverCapabilities.data_width];

    /* Check flash memory boundaries and alignment with minimal write size */
    rc  = is_range_valid(FLASH1_DEV, addr + byte_cnt);
    rc |= is_write_aligned(FLASH1_DEV, addr);
    rc |= is_write_aligned(FLASH1_DEV, byte_cnt);
    if (rc != 0) {
        return ARM_DRIVER_ERROR_PARAMETER;
    }

    /* Check if the flash area to write the data was erased previously */
    rc = is_flash_ready_to_write((const uint8_t*)start_addr, byte_cnt);
    if (rc != 0) {
        return ARM_DRIVER_ERROR;
    }

    /* Flash interface just emulated over SRAM, use memcpy */
    memcpy((void *)start_addr, data, byte_cnt);

    return cnt;
}

static int32_t ARM_Flash1_EraseSector(uint32_t addr)
{
    uint32_t start_addr = FLASH1_DEV->memory_base + addr;
    uint32_t rc = 0;

    rc  = is_range_valid(FLASH1_DEV, addr);
    rc |= is_sector_aligned(FLASH1_DEV, addr);
    if (rc != 0) {
        return ARM_DRIVER_ERROR_PARAMETER;
    }

    /* Flash interface just emulated over SRAM, use memset */
    memset((void *)start_addr,
           FLASH1_DEV->data->erased_value,
           FLASH1_DEV->data->sector_size);
    return ARM_DRIVER_OK;
}

static int32_t ARM_Flash1_EraseChip(void)
{
    uint32_t i;
    uint32_t addr = FLASH1_DEV->memory_base;
    int32_t rc = ARM_DRIVER_ERROR_UNSUPPORTED;

    /* Check driver capability erase_chip bit */
    if (DriverCapabilities.erase_chip == 1) {
        for (i = 0; i < FLASH1_DEV->data->sector_count; i++) {
            /* Flash interface just emulated over SRAM, use memset */
            memset((void *)addr,
                   FLASH1_DEV->data->erased_value,
                   FLASH1_DEV->data->sector_size);

            addr += FLASH1_DEV->data->sector_size;
            rc = ARM_DRIVER_OK;
        }
    }
    return rc;
}

static ARM_FLASH_STATUS ARM_Flash1_GetStatus(void)
{
    return FlashStatus;
}

static ARM_FLASH_INFO * ARM_Flash1_GetInfo(void)
{
    return FLASH1_DEV->data;
}

ARM_DRIVER_FLASH Driver_FLASH1 = {
    ARM_Flash1_GetVersion,
    ARM_Flash1_GetCapabilities,
    ARM_Flash1_Initialize,
    ARM_Flash1_Uninitialize,
    ARM_Flash1_PowerControl,
    ARM_Flash1_ReadData,
    ARM_Flash1_ProgramData,
    ARM_Flash1_EraseSector,
    ARM_Flash1_EraseChip,
    ARM_Flash1_GetStatus,
    ARM_Flash1_GetInfo
};
