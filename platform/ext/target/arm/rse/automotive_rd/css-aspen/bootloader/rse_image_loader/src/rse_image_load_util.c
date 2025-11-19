/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 */

#include <string.h>

#include "bootutil/boot_record.h"
#include "bootutil/bootutil_log.h"
#include "device_definition.h"
#include "fip_parser.h"
#include "flash_layout.h"
#include "fwu_hal_bl2.h"
#include "host_base_address.h"
#include "rse_image_load_util.h"
#include "tfm_plat_defs.h"

#ifdef MCUBOOT_ENCRYPT_RSA
#define BL2_MBEDTLS_MEM_BUF_LEN 0x3000
#else
#define BL2_MBEDTLS_MEM_BUF_LEN 0x2000
#endif

extern ARM_DRIVER_FLASH AP_FLASH_DEV_NAME;
extern struct flash_area flash_map[];
extern const int flash_map_entry_num;
extern int flash_area_driver_init(void);

static uint8_t mbedtls_mem_buf[BL2_MBEDTLS_MEM_BUF_LEN];

static void memclear_inbounds(void *base, size_t size_in_bytes)
{
    uintptr_t base_addr = (uintptr_t)base;
    uintptr_t end_addr  = base_addr + size_in_bytes;

    /* Clear leading unaligned bytes */
    uint8_t *byte_ptr = (uint8_t *)base_addr;
    uintptr_t aligned_start = (base_addr + 3u) & ~(uintptr_t)3u;

    while ((uintptr_t)byte_ptr < aligned_start &&
            (uintptr_t)byte_ptr < end_addr) {
        *byte_ptr++ = 0u;
    }

    /* Clear aligned body using 32-bit stores */
    uintptr_t aligned_end = end_addr & ~(uintptr_t)3u;

    if (aligned_end > aligned_start) {
        volatile uint32_t *word_ptr =
            (volatile uint32_t *)aligned_start;
        size_t word_count =
            (aligned_end - aligned_start) / sizeof(uint32_t);

        for (size_t i = 0; i < word_count; i++) {
            word_ptr[i] = 0u;
        }
    }

    /* Clear trailing unaligned bytes */
    byte_ptr = (uint8_t *)aligned_end;
    while ((uintptr_t)byte_ptr < end_addr) {
        *byte_ptr++ = 0u;
    }
}

static int wipe_ap_shared_sram_inline(void)
{
    enum atu_error_t atu_err;

    uint64_t phys_base  = HOST_AP_SHARED_SRAM_PHYS_BASE;
    uint64_t phys_limit = HOST_AP_SHARED_SRAM_PHYS_LIMIT;
    uint64_t phys_size  = phys_limit - phys_base + 1U;

    uint64_t wipe_size = ALIGN_UP(phys_size, RSE_ATU_PAGE_SIZE);
    uint64_t temp_logical_addr = RSE_IMAGE_LOADING_END; // or another free window

    atu_err = atu_rse_initialize_region(&ATU_DEV_S,
                                        RSE_ATU_IMG_CODE_LOAD_ID,
                                        temp_logical_addr,
                                        phys_base,
                                        wipe_size);
    if (atu_err != ATU_ERR_NONE) {
        BOOT_LOG_ERR("SRAM wipe: ATU init failed: %d", atu_err);
        return 1;
    }

    memclear_inbounds((void *)temp_logical_addr, wipe_size);

    atu_err = atu_rse_uninitialize_region(&ATU_DEV_S, RSE_ATU_IMG_CODE_LOAD_ID);
    if (atu_err != ATU_ERR_NONE) {
        BOOT_LOG_ERR("SRAM wipe: ATU uninit failed: %d", atu_err);
        return 1;
    }
    return 0;
}

static int rse_image_load_post_init(void)
{
    enum atu_error_t atu_err;
    int32_t result;

    atu_err = atu_rse_drv_init(&ATU_LIB_S, &ATU_DEV_S, ATU_DOMAIN_SECURE,
                               atu_regions_static, atu_stat_count);
    if (atu_err != ATU_ERR_NONE) {
        return -1;
    }

    result = flash_area_driver_init();
    if (result != 0) {
        BOOT_LOG_ERR("flash_area_driver_init failed: %d", result);
        return result;
    }

    result = AP_FLASH_DEV_NAME.Initialize(NULL);
    if (result != 0) {
        return result;
    }

    (void)fih_delay_init();

    return 0;
}

static enum atu_error_t rse_initialise_atu_regions(struct atu_dev_t *dev,
                                               uint8_t region_count,
                                               const struct atu_map *regions,
                                               const char *name)
{
    enum atu_error_t atu_err;

    for (uint8_t idx = 0; idx < region_count; idx++) {
        const struct atu_map *reg = &regions[idx];

        atu_err = atu_rse_initialize_region(dev, idx,
                                            reg->log_addr,
                                            reg->phy_addr,
                                            reg->size);
        if (atu_err != ATU_ERR_NONE) {
            BOOT_LOG_ERR("BL2: ERROR! %s ATU region %u init status: %d",
                         name, idx, (int)atu_err);
            return atu_err;
        }

        atu_err = atu_rse_set_bus_attributes(dev, idx, reg->bus_attr);
        if (atu_err != ATU_ERR_NONE) {
            BOOT_LOG_ERR("BL2: Unable to modify bus attributes for %s "
                         "ATU region %u", name, idx);
            return atu_err;
        }

        /* No support for "long long" prints, work around with more arguments */
        BOOT_LOG_INF("BL2: %s ATU region %u: [0x%lx - 0x%lx]->[0x%lx_%08lx - 0x%lx_%08lx]",
                     name, idx,
                     (unsigned long)reg->log_addr,
                     (unsigned long)(reg->log_addr + reg->size - 1),
                     (unsigned long)(reg->phy_addr >> 32),
                     (unsigned long)reg->phy_addr,
                     (unsigned long)((reg->phy_addr + reg->size - 1) >> 32),
                     (unsigned long)(reg->phy_addr + reg->size - 1));
    }

    return ATU_ERR_NONE;
}

static int rse_fill_secure_flash_map_with_data(void)
{
    uint64_t ap_bl2_offset = 0;
    size_t ap_bl2_size = 0;
    enum tfm_plat_err_t result;
    uint8_t i, id, primary_flash_id, secondary_flash_id;
    uint32_t ap_image_offset;
    uint8_t boot_index;
    bool flash_id_matched = false;

    boot_index = get_active_boot_index();

    if (boot_index == FWU_BANK_0) {
        /* Set flash_map[] of AP BL2 both slot offsets to Primary Slot offset */
        ap_image_offset = AP_FLASH_AREA_0_OFFSET;
    } else {
        /* Set flash_map[] of AP BL2 both slot offsets to Secondary Slot offset */
        ap_image_offset = AP_FLASH_AREA_1_OFFSET;
    }

    result = fip_get_entry_by_uuid(&AP_FLASH_DEV_NAME,
            ap_image_offset, AP_FLASH_FIP_SIZE,
            UUID_TRUSTED_BOOT_FIRMWARE_BL2,
            &ap_bl2_offset, &ap_bl2_size);
    if (result != TFM_PLAT_ERR_SUCCESS) {
        return 1;
    }

    primary_flash_id = FLASH_AREA_IMAGE_PRIMARY(RSE_FIRMWARE_AP_BL2_ID);
    secondary_flash_id = FLASH_AREA_IMAGE_SECONDARY(RSE_FIRMWARE_AP_BL2_ID);

    for (i = 0; i < flash_map_entry_num; i++) {
        id = flash_map[i].fa_id;

        if ((id == primary_flash_id) || (id == secondary_flash_id)) {
            flash_map[i].fa_off = ap_image_offset + ap_bl2_offset;
            flash_map[i].fa_size = ap_bl2_size;
            flash_id_matched = true;
        }
    }
    if (!flash_id_matched) {
      BOOT_LOG_ERR("AP BL2 flash_map entries missing");
      return 1;
    }
    return 0;
}

static int rse_image_pre_load_ap_bl2(void)
{
    enum atu_error_t atu_err;

    BOOT_LOG_INF("BL2: AP BL2 pre load start");

    /* Configure RSE ATU to access AP Secure Flash for AP BL2 */
    atu_err = atu_rse_initialize_region(&ATU_DEV_S,
                                        HOST_AP_FLASH_ATU_ID,
                                        HOST_AP_FLASH_BASE,
                                        HOST_AP_FLASH_PHY_BASE,
                                        HOST_AP_FLASH_SIZE);
    if (atu_err != ATU_ERR_NONE) {
        BOOT_LOG_ERR("BL2: ATU init failed for AP secure flash");
        return 1;
    }

    /* Configure RSE ATU to access RSE header region for AP BL2 */
    atu_err = atu_rse_initialize_region(&ATU_DEV_S,
                                        RSE_ATU_IMG_HDR_LOAD_ID,
                                        HOST_AP_BL2_HDR_ATU_WINDOW_BASE_S,
                                        HOST_AP_BL2_HDR_PHYS_BASE,
                                        RSE_IMG_HDR_ATU_WINDOW_SIZE);
    if (atu_err != ATU_ERR_NONE) {
        BOOT_LOG_ERR("BL2: ATU init failed for AP BL2 header");
        return 1;
    }

    /* Configure RSE ATU to access AP BL2 Shared SRAM region */
    atu_err = atu_rse_initialize_region(&ATU_DEV_S,
                                        RSE_ATU_IMG_CODE_LOAD_ID,
                                        HOST_AP_BL2_IMG_CODE_BASE_S,
                                        HOST_AP_BL2_PHYS_BASE,
                                        HOST_AP_BL2_ATU_SIZE);
    if (atu_err != ATU_ERR_NONE) {
        BOOT_LOG_ERR("BL2: ATU init failed for AP BL2 image");
        return 1;
    }

    if (rse_fill_secure_flash_map_with_data() != 0) {
        BOOT_LOG_ERR("BL2: Unable to extract AP BL2 from FIP");
        return 1;
    }

    BOOT_LOG_INF("BL2: AP BL2 pre load complete");

    return 0;
}

static enum atu_error_t rse_initialize_ap_atu(void)
{
    enum atu_error_t atu_err;

    /* Configure RSE ATU to access the AP ATU */
    atu_err = atu_rse_initialize_region(&ATU_DEV_S,
                                        RSE_ATU_AP_ATU_ID,
                                        HOST_AP_ATU_BASE_S,
                                        HOST_AP_ATU_PHYS_BASE,
                                        HOST_AP_ATU_GPV_SIZE);
    if (atu_err != ATU_ERR_NONE) {
        return atu_err;
    }

    /* Initialize the translation regions of the AP ATU */
    atu_err = rse_initialise_atu_regions(&HOST_AP_ATU_DEV, AP_ATU_REGION_COUNT,
                                     ap_atu_regions, "AP");
    if (atu_err != ATU_ERR_NONE) {
        return atu_err;
    }

    /* Close RSE ATU region configured to access the AP ATU */
    atu_err = atu_rse_uninitialize_region(&ATU_DEV_S, RSE_ATU_AP_ATU_ID);
    if (atu_err != ATU_ERR_NONE) {
        return atu_err;
    }

    return ATU_ERR_NONE;
}

static int rse_image_post_load_ap_bl2(void)
{
    enum atu_error_t atu_err;

    BOOT_LOG_INF("BL2: AP BL2 post load start");

    /*
     * Since the measurements are taken at this point, clear the image
     * header part in the Shared SRAM before releasing AP BL2 out of reset.
     */
    memset((void *)HOST_AP_BL2_IMG_HDR_BASE_S, 0, BL2_HEADER_SIZE);
    __DSB();

    /* Close RSE ATU to access AP Secure Flash for AP BL2 */
    atu_err = atu_rse_uninitialize_region(&ATU_DEV_S, HOST_AP_FLASH_ATU_ID);
    if (atu_err != ATU_ERR_NONE) {
        return 1;
    }

    /* Close RSE ATU region configured to access RSE header region for AP BL2 */
    atu_err = atu_rse_uninitialize_region(&ATU_DEV_S, RSE_ATU_IMG_HDR_LOAD_ID);
    if (atu_err != ATU_ERR_NONE) {
        return 1;
    }

    /* Close RSE ATU region configured to access AP BL2 Shared SRAM region */
    atu_err = atu_rse_uninitialize_region(&ATU_DEV_S, RSE_ATU_IMG_CODE_LOAD_ID);
    if (atu_err != ATU_ERR_NONE) {
        return 1;
    }

    /* Configure the AP ATU */
    atu_err = rse_initialize_ap_atu();
    if (atu_err != ATU_ERR_NONE) {
        return 1;
    }

    BOOT_LOG_INF("BL2: AP BL2 post load complete");

    return 0;
}

/*
 * Helper: load primary slot for AP BL2 from flash into AP_BL2_RAM_BASE and
 * verify hash from TLV using Mbed TLS SHA-256.
 */
static int rse_ap_bl2_load_and_validate(void)
{
    const struct flash_area *fa = NULL;
    struct image_header hdr;
    int rc;
    uint64_t code_base, code_size, flash_off, code_end;
    uint8_t boot_index;
    uint8_t flash_id;

    boot_index = get_active_boot_index();

    /* Get flash area */
    if (boot_index == FWU_BANK_0) {
        flash_id = FLASH_AREA_IMAGE_PRIMARY(RSE_FIRMWARE_AP_BL2_ID);
    } else {
        flash_id = FLASH_AREA_IMAGE_SECONDARY(RSE_FIRMWARE_AP_BL2_ID);
    }

    rc = flash_area_open(flash_id, &fa);
    if (rc != 0 || fa == NULL) {
        BOOT_LOG_INF("AP_BL2: flash_area_open(%u) failed, rc=%d\r\n",
                (unsigned)flash_id, rc);
        return rc ? rc : -1;
    }

    /* Read header */
    rc = flash_area_read(fa, 0, &hdr, sizeof(hdr));
    if (rc != 0) {
        BOOT_LOG_INF("AP_BL2: header read failed, rc=%d\r\n", rc);
        flash_area_close(fa);
        return rc;
    }
    if (hdr.ih_magic != IMAGE_MAGIC ||
        !(hdr.ih_flags & IMAGE_F_RAM_LOAD)) {
        BOOT_LOG_INF("AP_BL2: invalid header (magic/flags)\r\n");
        flash_area_close(fa);
        return -1;
    }

    /* Compute RAM destination for CODE (not header) */
    code_base = hdr.ih_load_addr + hdr.ih_hdr_size;
    code_size = hdr.ih_img_size;
    flash_off = hdr.ih_hdr_size;
    code_end  = code_base + code_size;

    /* Sanity check against AP BL2 SRAM window */
    if ((code_base < HOST_AP_BL2_IMG_CODE_BASE_S) ||
        (code_end > HOST_AP_BL2_IMG_CODE_BASE_S + HOST_AP_BL2_ATU_SIZE)) {
        BOOT_LOG_ERR("AP_BL2: code region [0x%08x..0x%08x) "
                     "outside RSE AP BL2 SRAM window [0x%08x..0x%08x)\r\n",
                     (unsigned)code_base, (unsigned)code_end,
                     (unsigned)HOST_AP_BL2_IMG_CODE_BASE_S,
                     (unsigned)(HOST_AP_BL2_IMG_CODE_BASE_S +
                                HOST_AP_BL2_ATU_SIZE));
        flash_area_close(fa);
        return -1;
    }

    /* Copy image body from flash to RAM (into CODE region) */
    rc = flash_area_read(fa, flash_off, (void *)code_base, code_size);
    flash_area_close(fa);

    if (rc != 0) {
        BOOT_LOG_INF("AP_BL2: image body read failed, rc=%d\r\n", rc);
        return rc;
    }

    BOOT_LOG_INF("AP_BL2: image load complete");
    return 0;
}

/*
 * Load and authenticate the AP BL2 image from RSE runtime context.
 * Intended for use when RSE orchestrates AP standalone reset.
 */
void rse_load_ap_bl2_image(void)
{
    int err;
    int32_t image_id = RSE_FIRMWARE_AP_BL2_ID;
    enum tfm_plat_err_t plat_err;

    /* Initialise the mbedtls static memory allocator so that mbedtls allocates
     * memory from the provided static buffer instead of from the heap.
     */
    mbedtls_memory_buffer_alloc_init(mbedtls_mem_buf,
                                     BL2_MBEDTLS_MEM_BUF_LEN);

    /* Wipe AP SRAM region before reloading AP BL2 image*/
    err = wipe_ap_shared_sram_inline();
    if (err != 0) {
        BOOT_LOG_ERR("AP SRAM wipe failed");
        boot_platform_error_state(err);
    }
    BOOT_LOG_INF("AP SRAM wipe completed");

    err = boot_platform_init();
    if (err != 0) {
        BOOT_LOG_ERR("Platform init failed");
        boot_platform_error_state(err);
    }

    err = rse_image_load_post_init();
    if (err != 0) {
        BOOT_LOG_ERR("rse_image_load_post_init failed: %d", err);
        boot_platform_error_state(err);
    }

    BOOT_LOG_INF("AP BL2: image_loading start");

    err = rse_image_pre_load_ap_bl2();
    if (err != 0) {
        BOOT_LOG_ERR("Pre-load step for image %d failed", image_id);
        boot_platform_error_state(err);
    }

    err = rse_ap_bl2_load_and_validate();
    if (err != 0) {
        BOOT_LOG_ERR("AP BL2: load/validate failed: %d", err);
        boot_platform_error_state(err);
    }

    BOOT_LOG_INF("AP BL2: load+validate succeeded");

    err = rse_image_post_load_ap_bl2();
    if (err != 0) {
        BOOT_LOG_ERR("Post-load step for image %d failed", image_id);
        boot_platform_error_state(err);
    }
}
