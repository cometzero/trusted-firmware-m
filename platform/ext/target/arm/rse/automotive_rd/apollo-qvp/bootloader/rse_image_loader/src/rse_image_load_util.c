/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 */

#include <psa/crypto.h>
#include <string.h>

#include "bootutil/boot_record.h"
#include "bootutil/bootutil_log.h"
#include "bootutil/image.h"
#include "device_definition.h"
#include "fip_parser.h"
#include "flash_layout.h"
#include "fwu_hal_bl2.h"
#include "host_base_address.h"
#include "mbedtls/asn1.h"
#include "rse_image_load_util.h"
#include "tlv.h"
#include "tfm_builtin_key_ids.h"
#include "tfm_plat_defs.h"
#include "tfm_plat_otp.h"

#ifdef MCUBOOT_ENCRYPT_RSA
#define BL2_MBEDTLS_MEM_BUF_LEN       (0x3000)
#else
#define BL2_MBEDTLS_MEM_BUF_LEN       (0x2000)
#endif

#define DER_TAG_SEQUENCE              (0x30U)
#define DER_TAG_INTEGER               (0x02U)
#define DER_TAG_BIT_STRING            (0x03U)
#define DER_UNUSED_BITS_ZERO          (0x00U)
#define EC_POINT_UNCOMPRESSED         (0x04U)
#define DER_MIN_ECDSA_SIG_LEN         (8U)
#define IMAGE_RAM_BASE                ((uintptr_t)0)
#define EXPECTED_ENC_LEN              (24)
#define MAX_CHUNK                     (1024U)

#define RSE_AP_BL2_HASH_ALG           PSA_ALG_SHA_256
#define RSE_AP_BL2_HASH_TLV_TYPE      IMAGE_TLV_SHA256
#define RSE_AP_BL2_HASH_LEN           (32U)

#define RSE_AP_BL2_KEY_TLV_TYPE       IMAGE_TLV_PUBKEY
#define RSE_AP_BL2_ROTPK_HASH_MAX_LEN (64U)

#define RSE_AP_BL2_SIG_TLV_TYPE       IMAGE_TLV_ECDSA_SIG
#define RSE_AP_BL2_KEY_TYPE           PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1)
#define RSE_AP_BL2_SIG_ALG            PSA_ALG_ECDSA(RSE_AP_BL2_HASH_ALG)
#define RSE_AP_BL2_KEY_BITS           (256U)

extern ARM_DRIVER_FLASH AP_FLASH_DEV_NAME;
extern struct flash_area flash_map[];
extern const int flash_map_entry_num;
extern int flash_area_driver_init(void);
extern enum tfm_otp_element_id_t rse_cm_get_bl2_rotpk(uint32_t image_id);
extern enum tfm_otp_element_id_t rse_dm_get_bl2_rotpk(uint32_t image_id);

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

    uint64_t phys_base  = HOST_AP_BL2_PHYS_BASE;
    uint64_t phys_size  = HOST_AP_BL2_ATU_SIZE;
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

static psa_status_t ctr_decrypt_chunk(psa_key_id_t aes_key,
                                      uint32_t payload_off_bytes,
                                      const uint8_t *in,
                                      size_t len,
                                      uint8_t *out)
{
    psa_cipher_operation_t op = psa_cipher_operation_init();
    uint8_t iv[16] = {0};
    size_t out_len = 0;
    psa_status_t st;

    /* Set counter = block index within encrypted region */
    uint32_t blk = (payload_off_bytes >> 4);
    iv[12] = (uint8_t)(blk >> 24);
    iv[13] = (uint8_t)(blk >> 16);
    iv[14] = (uint8_t)(blk >> 8);
    iv[15] = (uint8_t)(blk);

    st = psa_cipher_decrypt_setup(&op, aes_key, PSA_ALG_CTR);
    if (st != PSA_SUCCESS)
        goto out;

    st = psa_cipher_set_iv(&op, iv, sizeof(iv));
    if (st != PSA_SUCCESS)
        goto out;

    st = psa_cipher_update(&op, in, len, out, len, &out_len);
    if (st != PSA_SUCCESS)
        goto out;

    st = psa_cipher_finish(&op, out + out_len, len - out_len, &out_len);

out:
    psa_cipher_abort(&op);

    return st;
}

static int rse_ap_bl2_load_and_decrypt(const struct flash_area *fa,
                                       const struct image_header *hdr)
{
    uint32_t src_sz =
        (hdr->ih_hdr_size + hdr->ih_img_size + hdr->ih_protect_tlv_size);
    uint8_t *ram_dst = (void *)(IMAGE_RAM_BASE + hdr->ih_load_addr);
    struct image_tlv_info info;
    uint16_t len;
    int rc;
    uint32_t off;
    uint32_t chunk;
    uint32_t payload_size;
    uint32_t done = 0;
    uint32_t img_base, img_end;
    uint8_t *payload_ram;

#ifdef TFM_RUNTIME_DECRYPTION
    psa_key_id_t key_id = TFM_BUILTIN_KEY_ID_KEK;
    psa_status_t status = PSA_ERROR_INVALID_ARGUMENT;
    psa_key_id_t output_key_id = PSA_KEY_ID_NULL;
    psa_key_attributes_t key_attributes;
#endif /* TFM_RUNTIME_DECRYPTION */

    rc = flash_area_read(fa, src_sz, &info, sizeof(info));
    if (rc != 0) {
        BOOT_LOG_INF("AP_BL2: flash_area_read(image) failed rc:%d\r\n", rc);
        return rc;
    }
    if (info.it_magic != IMAGE_TLV_INFO_MAGIC &&
            info.it_magic != IMAGE_TLV_PROT_INFO_MAGIC) {
        BOOT_LOG_ERR("AP_BL2: invalid TLV magic 0x%x\r\n", info.it_magic);
        return -1;
    }
    src_sz += info.it_tlv_tot;

    /* Compute RAM destination for full image (header + payload + TLVs) */
    img_base = IMAGE_RAM_BASE + hdr->ih_load_addr + hdr->ih_hdr_size;
    img_end  = img_base + src_sz;

    /* Sanity check against AP BL2 SRAM window */
    if ((img_base < HOST_AP_BL2_IMG_CODE_BASE_S) ||
            (img_end  > HOST_AP_BL2_IMG_CODE_BASE_S + HOST_AP_BL2_ATU_SIZE)) {

        BOOT_LOG_ERR("AP_BL2: image region [0x%08x..0x%08x) "
                "outside RSE AP BL2 SRAM window [0x%08x..0x%08x)\r\n",
                (unsigned)img_base, (unsigned)img_end,
                (unsigned)HOST_AP_BL2_IMG_CODE_BASE_S,
                (unsigned)(HOST_AP_BL2_IMG_CODE_BASE_S +
                    HOST_AP_BL2_ATU_SIZE));

        return -1;
    }
    /* Load full image into RAM */
    rc = flash_area_read(fa, 0, ram_dst, src_sz);
    if (rc != 0) {
        BOOT_LOG_INF("AP_BL2: flash_area_read(image) failed rc:%d\r\n", rc);
        return rc;
    }

#ifdef TFM_RUNTIME_DECRYPTION
    /* Locate encrypted key TLV */
    rc = rse_find_tlv_by_type(hdr, ram_dst, false, IMAGE_TLV_ENC_KW, &off, &len);
    if (rc != 0) {
        BOOT_LOG_INF("AP_BL2: rse_find_tlv_by_type failed rc:%d\r\n", rc);
        return rc;
    }
    if (len != EXPECTED_ENC_LEN) {
        return -1;
    }

    const uint8_t *wrapped_key = (const uint8_t *)(ram_dst + off);

    /* Unwrap AES key */
    key_attributes = psa_key_attributes_init();
    psa_set_key_algorithm(&key_attributes, PSA_ALG_CTR);
    psa_set_key_type(&key_attributes, PSA_KEY_TYPE_AES);
    psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_DECRYPT);

    status = psa_unwrap_key(&key_attributes, key_id, PSA_ALG_ECB_NO_PADDING,
            wrapped_key, EXPECTED_ENC_LEN, &output_key_id);
    if (status != PSA_SUCCESS) {
        BOOT_LOG_INF("AP_BL2: key unwrap failed status:%d", status);
        return status;
    }

    /* Decrypt payload in place */
    payload_size = hdr->ih_img_size;
    payload_ram = ram_dst + hdr->ih_hdr_size;

    while (done < payload_size) {
        chunk = payload_size - done;
        if (chunk > MAX_CHUNK) {
            chunk = MAX_CHUNK;
        }
        /* payload offset (bytes) relative to start of payload region */
        rc = ctr_decrypt_chunk(output_key_id, done, (payload_ram + done), chunk,
                (payload_ram + done));
        if (rc != PSA_SUCCESS) {
            BOOT_LOG_ERR("AP BL2: CTR decrypt failed st=%d at off=%u", rc, done);
            psa_destroy_key(output_key_id);
            return -1;
        }
        done += chunk;
    }
    psa_destroy_key(output_key_id);

    BOOT_LOG_INF("AP_BL2: load and decrypt complete");
#else
    BOOT_LOG_INF("AP_BL2: load complete");
#endif /* TFM_RUNTIME_DECRYPTION */
    return 0;
}

static int rse_hash_compute_chunked(psa_algorithm_t alg,
                                    const uint8_t *buf,
                                    uint32_t len,
                                    uint8_t *hash,
                                    size_t hash_size,
                                    size_t *hash_len)
{
    uint32_t done = 0;
    uint32_t chunk;
    psa_hash_operation_t hash_op;
    psa_status_t status = PSA_ERROR_INVALID_ARGUMENT;

    if (!buf || !hash || !hash_len) {
        BOOT_LOG_ERR("AP_BL2: hash input invalid");
        return -1;
    }

    hash_op = psa_hash_operation_init();

    status = psa_hash_setup(&hash_op, alg);
    if (status != PSA_SUCCESS) {
        BOOT_LOG_ERR("AP_BL2: psa_hash_setup failed: %d", status);
        goto out;
    }

    while (done < len) {
        chunk = len - done;
        if (chunk > MAX_CHUNK) {
            chunk = MAX_CHUNK;
        }

        status = psa_hash_update(&hash_op, buf + done, chunk);
        if (status != PSA_SUCCESS) {
            BOOT_LOG_ERR("AP_BL2: psa_hash_update failed at %u: %d",
                         (unsigned)done, status);
            goto out;
        }

        done += chunk;
    }

    status = psa_hash_finish(&hash_op, hash, hash_size, hash_len);
    if (status != PSA_SUCCESS) {
        BOOT_LOG_ERR("AP_BL2: psa_hash_finish failed: %d", status);
    }

out:
    (void)psa_hash_abort(&hash_op);
    return (status == PSA_SUCCESS) ? 0 : -1;
}

static int rse_get_rotpk_otp_id(uint32_t image_index,
                                enum tfm_otp_element_id_t *otp_id)
{
    enum tfm_otp_element_id_t cm_id;
    enum tfm_otp_element_id_t dm_id;

    if (!otp_id) {
        BOOT_LOG_ERR("AP_BL2: RoTPK OTP output pointer NULL");
        return -1;
    }

    cm_id = rse_cm_get_bl2_rotpk(image_index);
    dm_id = rse_dm_get_bl2_rotpk(image_index);

    if (cm_id != PLAT_OTP_ID_INVALID) {
        /* Give CM precedence */
        *otp_id = cm_id;
    } else if (dm_id != PLAT_OTP_ID_INVALID) {
        *otp_id = dm_id;
    } else {
        BOOT_LOG_ERR("AP_BL2: no RoTPK OTP ID for image %u",
                     (unsigned)image_index);
        return -1;
    }

    return 0;
}

static int rse_get_rotpk_hash_from_otp(uint8_t image_id,
                                       uint8_t *rotpk_hash,
                                       uint32_t *rotpk_hash_len)
{
    enum tfm_plat_err_t plat_err;
    enum tfm_otp_element_id_t otp_id;
    size_t otp_size;

    if (!rotpk_hash || !rotpk_hash_len) {
        BOOT_LOG_ERR("AP_BL2: RoTPK hash buffers NULL");
        return -1;
    }

    if (rse_get_rotpk_otp_id(image_id, &otp_id) != 0) {
        BOOT_LOG_ERR("AP_BL2: RoTPK OTP ID lookup failed");
        return -1;
    }

    plat_err = tfm_plat_otp_read(otp_id,
                                 *rotpk_hash_len, rotpk_hash);
    if (plat_err != TFM_PLAT_ERR_SUCCESS) {
        BOOT_LOG_ERR("AP_BL2: RoTPK OTP read failed: %d", (int)plat_err);
        return -1;
    }

    plat_err = tfm_plat_otp_get_size(otp_id,
                                     &otp_size);
    if (plat_err != TFM_PLAT_ERR_SUCCESS) {
        BOOT_LOG_ERR("AP_BL2: RoTPK OTP size read failed: %d", (int)plat_err);
        return -1;
    }

    *rotpk_hash_len = (uint32_t)otp_size;
    return 0;
}

static int rse_ap_bl2_verify_pubkey_trust(const uint8_t *pubkey,
                                          uint16_t pubkey_len)
{
    uint8_t pubkey_hash[RSE_AP_BL2_ROTPK_HASH_MAX_LEN];
    uint8_t rotpk_hash[RSE_AP_BL2_ROTPK_HASH_MAX_LEN];
    size_t pubkey_hash_len = 0;
    uint32_t rotpk_hash_len = sizeof(rotpk_hash);
    psa_algorithm_t hash_alg;

    if (!pubkey || pubkey_len == 0U) {
        return -1;
    }

    if (rse_get_rotpk_hash_from_otp((uint8_t)RSE_FIRMWARE_AP_BL2_ID,
                                    rotpk_hash, &rotpk_hash_len) != 0) {
        BOOT_LOG_ERR("AP_BL2: RoTPK hash OTP read failed");
        return -1;
    }

    if (rotpk_hash_len == RSE_AP_BL2_HASH_LEN) {
        hash_alg = PSA_ALG_SHA_256;
    } else {
        BOOT_LOG_ERR("AP_BL2: unsupported RoTPK hash len %u", (unsigned)rotpk_hash_len);
        return -1;
    }

    if (rse_hash_compute_chunked(hash_alg, pubkey, pubkey_len,
                                 pubkey_hash, sizeof(pubkey_hash),
                                 &pubkey_hash_len) != 0) {
        BOOT_LOG_ERR("AP_BL2: pubkey hash compute failed");
        return -1;
    }

    if (pubkey_hash_len != rotpk_hash_len ||
        memcmp(pubkey_hash, rotpk_hash, rotpk_hash_len) != 0) {
        BOOT_LOG_ERR("AP_BL2: pubkey trust check failed");
        return -1;
    }

    return 0;
}

/* Parse Subject public key info and extract EC key as uncompressed point:
 * 0x04 || X || Y.
 */
static int rse_ecdsa_extract_pubkey_from_spki(const uint8_t *spki,
                                              size_t spki_len,
                                              uint8_t *raw_key,
                                              size_t raw_key_size,
                                              size_t *raw_key_len)
{
    const uint8_t *p;
    const uint8_t *end;
    const uint8_t *seq_end;
    unsigned char *pos;
    size_t len;
    size_t ec_bytes = (RSE_AP_BL2_KEY_BITS / 8U);
    size_t expected = 1U + (2U * ec_bytes);

    if (!spki || !raw_key || !raw_key_len || raw_key_size < expected) {
        BOOT_LOG_ERR("AP_BL2: invalid SPKI parse input");
        return -1;
    }

    p = spki;
    end = spki + spki_len;

    if ((p >= end) || (*p++ != DER_TAG_SEQUENCE)) {
        return -1;
    }
    pos = (unsigned char *)p;
    if (mbedtls_asn1_get_len(&pos, (const unsigned char *)end, &len) != 0) {
        return -1;
    }
    p = (const uint8_t *)pos;
    if ((size_t)(end - p) < len) {
        return -1;
    }
    seq_end = p + len;

    /* AlgorithmIdentifier sequence */
    if ((p >= seq_end) || (*p++ != DER_TAG_SEQUENCE)) {
        return -1;
    }
    pos = (unsigned char *)p;
    if (mbedtls_asn1_get_len(&pos, (const unsigned char *)seq_end, &len) != 0) {
        return -1;
    }
    p = (const uint8_t *)pos;
    if ((size_t)(seq_end - p) < len) {
        return -1;
    }
    p += len;

    /* subjectPublicKey BIT STRING */
    if ((p >= seq_end) || (*p++ != DER_TAG_BIT_STRING)) {
        return -1;
    }
    pos = (unsigned char *)p;
    if (mbedtls_asn1_get_len(&pos, (const unsigned char *)seq_end, &len) != 0) {
        return -1;
    }
    p = (const uint8_t *)pos;
    if ((size_t)(seq_end - p) < len || len < 2U) {
        return -1;
    }

    if (*p++ != DER_UNUSED_BITS_ZERO) { /* unused bits must be 0 */
        return -1;
    }
    len--;

    if (len != expected || p[0] != EC_POINT_UNCOMPRESSED) {
        return -1;
    }

    memcpy(raw_key, p, len);
    *raw_key_len = len;
    return 0;
}

/*
 * Parse one DER TLV item with short-form length only:
 *   [ tag ][ 1-byte length ][ value... ]
 * Advances *p to the next item and returns a pointer/length for the value.
 */
static int rse_der_read_tlv_one_byte_len(const uint8_t **p,
                                         const uint8_t *end,
                                         uint8_t expected_tag,
                                         const uint8_t **val,
                                         size_t *val_len)
{
    uint8_t len_byte;

    if (!p || !*p || !val || !val_len || ((size_t)(end - *p) < 2U)) {
        BOOT_LOG_ERR("AP_BL2: invalid DER TLV input");
        return -1;
    }

    if (*(*p)++ != expected_tag) {
        return -1;
    }

    len_byte = *(*p)++;
    if ((size_t)(end - *p) < (size_t)len_byte) {
        return -1;
    }

    *val = *p;
    *val_len = (size_t)len_byte;
    *p += len_byte;
    return 0;
}

/*
 * Convert DER INTEGER bytes into a fixed-size scalar.
 */
static int rse_ecdsa_scalar_to_fixed(const uint8_t *scalar,
                                     size_t scalar_len,
                                     uint8_t *dst,
                                     size_t dst_len)
{
    if (!scalar || !dst || scalar_len == 0U) {
        BOOT_LOG_ERR("AP_BL2: invalid ECDSA scalar input");
        return -1;
    }

    /* DER INTEGER can include one leading 0x00 to force positive sign. */
    if (scalar_len > dst_len) {
        if ((scalar_len != (dst_len + 1U)) || (scalar[0] != 0x00U)) {
            BOOT_LOG_ERR("AP_BL2: ECDSA scalar length invalid");
            return -1;
        }
        scalar++;
        scalar_len--;
    }

    memset(dst, 0, dst_len);
    memcpy(dst + (dst_len - scalar_len), scalar, scalar_len);
    return 0;
}

/*
 * Convert ECDSA signature from DER sequence:
 *   SEQUENCE { INTEGER r, INTEGER s }
 * into raw fixed-width form:
 *   r || s
 * where each scalar is padded to curve-byte width.
 */
static int rse_ecdsa_der_sig_to_raw(const uint8_t *der, uint16_t der_len,
                                    uint8_t *raw, size_t raw_len)
{
    size_t ec_bytes = (size_t)(RSE_AP_BL2_KEY_BITS / 8U);
    const uint8_t *p;
    const uint8_t *end;
    const uint8_t *seq_val;
    const uint8_t *r_val;
    const uint8_t *s_val;
    size_t seq_len;
    size_t r_len;
    size_t s_len;

    if (!der || !raw || raw_len != (2U * ec_bytes) || der_len < DER_MIN_ECDSA_SIG_LEN) {
        BOOT_LOG_ERR("AP_BL2: invalid DER signature input");
        return -1;
    }

    p = der;
    end = der + der_len;

    if (rse_der_read_tlv_one_byte_len(&p, end, DER_TAG_SEQUENCE, &seq_val, &seq_len) != 0) {
        return -1;
    }
    if ((seq_val != (p - seq_len)) || (p != end)) {
        BOOT_LOG_ERR("AP_BL2: DER signature sequence invalid");
        return -1;
    }

    p = seq_val;
    if (rse_der_read_tlv_one_byte_len(&p, end, DER_TAG_INTEGER, &r_val, &r_len) != 0) {
        return -1;
    }
    if (rse_der_read_tlv_one_byte_len(&p, end, DER_TAG_INTEGER, &s_val, &s_len) != 0) {
        return -1;
    }
    if (p != end) {
        return -1;
    }

    if (rse_ecdsa_scalar_to_fixed(r_val, r_len, raw, ec_bytes) != 0) {
        return -1;
    }
    if (rse_ecdsa_scalar_to_fixed(s_val, s_len, raw + ec_bytes, ec_bytes) != 0) {
        return -1;
    }

    return 0;
}

static int rse_ap_bl2_verify_signature(const uint8_t *pubkey,
                                       uint16_t pubkey_len,
                                       const uint8_t *sig,
                                       uint16_t sig_len,
                                       const uint8_t *hash,
                                       size_t hash_len)
{
    psa_key_id_t pubkey_id = PSA_KEY_ID_NULL;
    psa_key_attributes_t key_attr = psa_key_attributes_init();
    psa_status_t status;
    psa_status_t destroy_status;
    uint8_t sig_raw[(RSE_AP_BL2_KEY_BITS / 8U) * 2U];
    uint8_t pubkey_raw[1U + ((RSE_AP_BL2_KEY_BITS / 8U) * 2U)];
    const uint8_t *sig_ptr = sig;
    const uint8_t *pubkey_ptr = pubkey;
    size_t sig_len_used = sig_len;
    size_t pubkey_len_used = pubkey_len;

    if (!pubkey || !sig || !hash) {
        BOOT_LOG_ERR("AP_BL2: signature verify input invalid");
        return -1;
    }

    psa_set_key_type(&key_attr, RSE_AP_BL2_KEY_TYPE);
    psa_set_key_usage_flags(&key_attr, PSA_KEY_USAGE_VERIFY_HASH);
    psa_set_key_algorithm(&key_attr, RSE_AP_BL2_SIG_ALG);
    psa_set_key_bits(&key_attr, RSE_AP_BL2_KEY_BITS);

    if (rse_ecdsa_extract_pubkey_from_spki(pubkey, pubkey_len,
                                           pubkey_raw, sizeof(pubkey_raw),
                                           &pubkey_len_used) != 0) {
        BOOT_LOG_ERR("AP_BL2: ECDSA pubkey SPKI parse failed");
        return -1;
    }
    pubkey_ptr = pubkey_raw;

    if (rse_ecdsa_der_sig_to_raw(sig, sig_len, sig_raw, sizeof(sig_raw)) != 0) {
        BOOT_LOG_ERR("AP_BL2: ECDSA signature DER parse failed");
        return -1;
    }
    sig_ptr = sig_raw;
    sig_len_used = sizeof(sig_raw);

    status = psa_import_key(&key_attr, pubkey_ptr, pubkey_len_used, &pubkey_id);
    if (status != PSA_SUCCESS) {
        BOOT_LOG_ERR("AP_BL2: pubkey import failed: %d", status);
        return -1;
    }

    status = psa_verify_hash(pubkey_id, RSE_AP_BL2_SIG_ALG,
                             hash, hash_len, sig_ptr, sig_len_used);
    destroy_status = psa_destroy_key(pubkey_id);
    if (destroy_status != PSA_SUCCESS) {
        BOOT_LOG_ERR("AP_BL2: key destroy failed: %d", destroy_status);
        if (status == PSA_SUCCESS) {
            return -1;
        }
    }
    if (status != PSA_SUCCESS) {
        BOOT_LOG_ERR("AP_BL2: signature verify failed: %d", status);
        return -1;
    }

    return 0;
}

static int rse_ap_bl2_validate_loaded_image(const struct image_header *hdr)
{
    uint8_t computed_hash[RSE_AP_BL2_HASH_LEN];
    size_t computed_hash_len = 0;
    uint32_t hash_input_len;
    uint32_t tlv_off = 0;
    const uint8_t *img_base;
    const uint8_t *tlv_hash = NULL;
    const uint8_t *tlv_pubkey = NULL;
    const uint8_t *tlv_sig = NULL;
    uint16_t tlv_hash_len = 0;
    uint16_t tlv_pubkey_len = 0;
    uint16_t tlv_sig_len = 0;

    if (!hdr) {
        BOOT_LOG_ERR("AP_BL2: image header is NULL");
        return -1;
    }

    hash_input_len = hdr->ih_hdr_size + hdr->ih_img_size + hdr->ih_protect_tlv_size;
    img_base = (const uint8_t *)(IMAGE_RAM_BASE + hdr->ih_load_addr);

    if (rse_hash_compute_chunked(RSE_AP_BL2_HASH_ALG,
                                 img_base, hash_input_len,
                                 computed_hash, sizeof(computed_hash),
                                 &computed_hash_len) != 0) {
        BOOT_LOG_ERR("AP_BL2: image hash compute failed");
        return -1;
    }
    if (computed_hash_len != sizeof(computed_hash)) {
        BOOT_LOG_ERR("AP_BL2: image hash len mismatch");
        return -1;
    }

    if (rse_find_tlv_by_type(hdr, img_base, false, RSE_AP_BL2_HASH_TLV_TYPE,
                             &tlv_off, &tlv_hash_len) != RSE_TLV_FOUND ||
        tlv_hash_len != RSE_AP_BL2_HASH_LEN) {
        BOOT_LOG_ERR("AP_BL2: hash TLV missing/invalid");
        return -1;
    }
    tlv_hash = img_base + tlv_off;
    if (memcmp(computed_hash, tlv_hash, sizeof(computed_hash)) != 0) {
        BOOT_LOG_ERR("AP_BL2: hash validation failed");
        return -1;
    }

    if (rse_find_tlv_by_type(hdr, img_base, false, RSE_AP_BL2_KEY_TLV_TYPE,
                             &tlv_off, &tlv_pubkey_len) != RSE_TLV_FOUND ||
        tlv_pubkey_len == 0U) {
        BOOT_LOG_ERR("AP_BL2: pubkey TLV missing");
        return -1;
    }
    tlv_pubkey = img_base + tlv_off;
    if (rse_ap_bl2_verify_pubkey_trust(tlv_pubkey, tlv_pubkey_len) != 0) {
        return -1;
    }

    if (rse_find_tlv_by_type(hdr, img_base, false, RSE_AP_BL2_SIG_TLV_TYPE,
                             &tlv_off, &tlv_sig_len) != RSE_TLV_FOUND ||
        tlv_sig_len == 0U) {
        BOOT_LOG_ERR("AP_BL2: signature TLV missing");
        return -1;
    }
    tlv_sig = img_base + tlv_off;
    if (rse_ap_bl2_verify_signature(tlv_pubkey, tlv_pubkey_len,
                                    tlv_sig, tlv_sig_len,
                                    computed_hash, computed_hash_len) != 0) {
        return -1;
    }

    BOOT_LOG_INF("AP_BL2: hash + signature validation passed");
    return 0;
}

/*
 * Helper: load primary slot for AP BL2 from flash into AP_BL2_RAM_BASE and
 * validate hash + signature TLVs after runtime load/decrypt.
 */
static int rse_ap_bl2_load_and_validate(void)
{
    const struct flash_area *fa = NULL;
    struct image_header hdr;
    int rc;
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

    rc = rse_ap_bl2_load_and_decrypt(fa, &hdr);
    if (rc != 0) {
        BOOT_LOG_INF("AP_BL2: load and decrypt failed");
        flash_area_close(fa);
        return rc;
    }

    rc = rse_ap_bl2_validate_loaded_image(&hdr);
    if (rc != 0) {
        BOOT_LOG_INF("AP_BL2: validation failed");
        flash_area_close(fa);
        return rc;
    }

    flash_area_close(fa);
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
