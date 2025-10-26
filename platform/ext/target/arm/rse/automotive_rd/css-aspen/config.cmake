#-------------------------------------------------------------------------------
# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
#-------------------------------------------------------------------------------

# Note: Cached varaibles always take the first value set, while normal
# variables always take the last.

set(ATTEST_KEY_BITS                     256      CACHE STRING  "The size of the initial attestation key in bits")
set(CONFIG_TFM_BOOT_STORE_MEASUREMENTS  ON       CACHE BOOL    "Store measurement values from all the boot stages")
set(CONFIG_TFM_SPM_BACKEND              IPC      CACHE STRING  "The SPM backend")
set(MCUBOOT_IMAGE_NUMBER                5        CACHE STRING  "Number of images supported by MCUBoot")
set(MULTI_RSE_TOPOLOGY_FILE             ${CMAKE_CURRENT_LIST_DIR}/sfcp/sfcp_topology.tgf CACHE STRING "Topology file for RSE to RSE BL1 communication")
set(PLATFORM_DEFAULT_MEASUREMENT_SLOTS  OFF      CACHE BOOL    "Use default Measured Boot slots")
set(PLATFORM_HAS_BOOT_DMA               OFF      CACHE BOOL    "Enable dma support for memory transactions for bootloader")
set(PLATFORM_HAS_PS_NV_OTP_COUNTERS     ON       CACHE BOOL    "Platform supports nvm counters for PS in OTP")
set(PLATFORM_HOST_HAS_SI_CL             ON       CACHE BOOL    "Enable Safety Island Cluster (SI CL) support")
set(PLATFORM_RSE_HAS_ATU_OWNERSHIP      ON       CACHE BOOL    "Enable RSE ATU ownership")
set(PLAT_MHU_VERSION                    3        CACHE STRING  "Supported MHU version by platform")
set(RSE_HAS_EXPANSION_PERIPHERALS       ON       CACHE BOOL    "Whether RSE has sub-platform specific peripherals in the expansion layer")
set(RSE_SUBPLATFORM_BOOT_MEASUREMENTS   ON       CACHE BOOL    "Use RSE subplatform boot measurement slot definition")
set(RSE_USE_HOST_FLASH                  OFF      CACHE BOOL    "Enable RSE using the host flash.")
set(RSE_USE_HOST_UART                   ON       CACHE BOOL    "Whether RSE should use the UART from the host system (opposed to dedicated UART private to RSE)")
set(RSE_USE_LOCAL_UART                  OFF      CACHE BOOL    "Whether RSE should use the local UART dedicated to RSE")
set(TFM_ATTESTATION_SCHEME              "PSA"    CACHE STRING  "Attestation scheme to use [OFF, PSA, CCA, DPE]")
set(TFM_BL1_MEMORY_MAPPED_FLASH         OFF      CACHE BOOL    "Whether BL1 can directly access flash content")
set(TFM_EXTRAS_REPO_EXTRA_MANIFEST_LIST "partitions/scmi/scmi_comms_manifest_list.yaml;partitions/measured_boot/measured_boot_manifest_list.yaml" CACHE STRING "List of extra secure partition manifests")
set(TFM_EXTRAS_REPO_EXTRA_PARTITIONS    "scmi;measured_boot"    CACHE STRING "List of extra secure partition directory name(s)")
set(TFM_LOAD_NS_IMAGE                   OFF      CACHE BOOL    "Whether to load an NS image")
set(TFM_MANIFEST_LIST                   "${CMAKE_CURRENT_LIST_DIR}/manifest/tfm_manifest_list.yaml" CACHE PATH "Platform specific Secure Partition manifests file")
set(TFM_MBEDCRYPTO_PLATFORM_EXTRA_CONFIG_PATH "" CACHE PATH    "Config to append to standard Mbed Crypto config, used by platforms to configure feature support")
set(TFM_PARTITION_INITIAL_ATTESTATION   ON       CACHE BOOL    "Enable Initial Attestation partition")
set(TFM_PARTITION_INTERNAL_TRUSTED_STORAGE ON    CACHE BOOL    "Enable Internal Trusted Storage partition")
set(TFM_PARTITION_MEASURED_BOOT         ON       CACHE BOOL    "Enable Measured Boot partition")
set(TFM_PARTITION_PROTECTED_STORAGE     ON       CACHE BOOL    "Enable Protected Storage partition")
set(TFM_PARTITION_SCMI_COMMS            ON       CACHE BOOL    "Enable SCMI Comms partition")
set(SFCP_ATU_REGION_SIZE                0x20000  CACHE STRING  "The size of the carveout region")
set(SFCP_NUMBER_NODES                   3        CACHE STRING  "Amount of nodes in the SFCP system, by default equal to number of RSEs")
set(RSE_COMMS_ATU_REGION_SIZE           0x20000  CACHE STRING  "The size of the carveout region")
set(RSE_COMMS_NUMBER_NODES              3        CACHE STRING  "Amount of nodes in the RSE comms system, by default equal to number of RSEs")
set(PLATFORM_HAS_FIRMWARE_UPDATE_SUPPORT   ON    CACHE BOOL    "Whether the platform has firmware update support")
set(TFM_PARTITION_FIRMWARE_UPDATE       ON       CACHE BOOL    "Enable firmware update partition")
set(TFM_FWU_BOOTLOADER_LIB              "${CMAKE_SOURCE_DIR}/platform/ext/target/arm/rse/automotive_rd/css-aspen/fwu"   CACHE STRING    "Bootloader configure file for Firmware Update partition")
set(FWU_DEVICE_CONFIG_FILE              "${CMAKE_BINARY_DIR}/generated/interface/include/psa/fwu_config.h"              CACHE STRING    "The device configuration file for Firmware Update partition")
set(NR_OF_FW_BANKS                      2        CACHE STRING  "Number of firmware banks")
set(NR_OF_IMAGES_IN_FW_BANK             4        CACHE STRING  "Number of images per firmware bank")
set(FWU_SUPPORT_TRIAL_STATE             ON       CACHE BOOL    "Device support TRIAL component state.")
set(TFM_CONFIG_FWU_MAX_WRITE_SIZE       4096     CACHE STRING  "The maximum permitted size for block in psa_fwu_write, in bytes.")
set(MCUBOOT_CUSTOM_DATA_SHARING_FUNCTION   ON    CACHE BOOL    "Enable platform-defined data sharing function between the bootloader and runtime firmware")
set(FWU_DEVICE_IMPL_INFO_DEF_FILE       "${CMAKE_CURRENT_LIST_DIR}/fwu/tfm_fwu_impl_info.h"    CACHE STRING    "The platform specific header file defining psa_fwu_impl_info_t structure")

# Once all cache options are set, set common options as fallback
include(${CMAKE_CURRENT_LIST_DIR}/../../common/config.cmake)
