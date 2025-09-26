#-------------------------------------------------------------------------------
# SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

if(NOT TFM_PARTITION_FIRMWARE_UPDATE)
    return()
endif()

cmake_minimum_required(VERSION 3.21)

add_library(tfm_bootloader_fwu_abstraction INTERFACE)

target_include_directories(tfm_bootloader_fwu_abstraction
    INTERFACE
        .
        ${CMAKE_SOURCE_DIR}/secure_fw/partitions/firmware_update/bootloader
        ${CMAKE_SOURCE_DIR}/platform/ext/target/arm/rse/common/soft_crc
        ${CMAKE_SOURCE_DIR}/platform/ext/target/arm/rse/common/fip_parser
        ${CMAKE_SOURCE_DIR}/platform/ext/target/arm/rse/common
        ${CMAKE_SOURCE_DIR}/secure_fw/partitions/lib/runtime/include
)

target_sources(tfm_bootloader_fwu_abstraction
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/tfm_bootloader_fwu_abstraction.c
        ${CMAKE_CURRENT_SOURCE_DIR}/fwu_metadata.c
        ${CMAKE_CURRENT_SOURCE_DIR}/fwu_private_metadata.c
        ${CMAKE_CURRENT_SOURCE_DIR}/fwu_flash.c
        ${CMAKE_SOURCE_DIR}/platform/ext/target/arm/rse/common/soft_crc/soft_crc.c
)

target_link_libraries(tfm_bootloader_fwu_abstraction
    INTERFACE
        tfm_log
)

target_link_libraries(tfm_psa_rot_partition_fwu
    PRIVATE
        tfm_bootloader_fwu_abstraction
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fwu_config.h.in
    ${CMAKE_BINARY_DIR}/generated/interface/include/psa/fwu_config.h
    @ONLY)

# Export FWU configuration
target_compile_definitions(tfm_config
    INTERFACE
        FWU_DEVICE_CONFIG_FILE="${FWU_DEVICE_CONFIG_FILE}"
        $<$<BOOL:${FWU_DEVICE_IMPL_INFO_DEF_FILE}>:FWU_DEVICE_IMPL_INFO_DEF_FILE="${FWU_DEVICE_IMPL_INFO_DEF_FILE}">
)
