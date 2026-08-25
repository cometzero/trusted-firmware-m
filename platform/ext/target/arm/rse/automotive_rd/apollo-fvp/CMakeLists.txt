#-------------------------------------------------------------------------------
# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
#-------------------------------------------------------------------------------

cmake_policy(SET CMP0076 NEW)
set(CMAKE_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../common common)

target_compile_definitions(platform_common_interface
                           INTERFACE
                               APOLLO_FVP_VARIANT_FVP=0
                               APOLLO_FVP_VARIANT_RTL=1
                               APOLLO_FVP_RTL_VARIANT_FPGA=0
                               APOLLO_FVP_RTL_VARIANT_EMU=1
                               $<$<BOOL:${RSE_GPIO_SELF_TEST}>:RSE_GPIO_SELF_TEST>)

if (TFM_PLATFORM_VARIANT STREQUAL "fvp")
      target_compile_definitions(platform_common_interface
                                 INTERFACE TFM_PLATFORM_VARIANT=0)
elseif (TFM_PLATFORM_VARIANT STREQUAL "rtl")
      target_compile_definitions(platform_common_interface
                                 INTERFACE TFM_PLATFORM_VARIANT=1)

    if (NOT DEFINED TFM_RTL_VARIANT)
        message(FATAL_ERROR "Please set TFM_RTL_VARIANT when TFM_PLATFORM_VARIANT is rtl\n")
    elseif (TFM_RTL_VARIANT STREQUAL "fpga")
        target_compile_definitions(platform_common_interface
                                    INTERFACE TFM_RTL_VARIANT=APOLLO_FVP_RTL_VARIANT_FPGA)
    elseif (TFM_RTL_VARIANT STREQUAL "emu")
        target_compile_definitions(platform_common_interface
                                    INTERFACE TFM_RTL_VARIANT=APOLLO_FVP_RTL_VARIANT_EMU)
    else()
        message(FATAL_ERROR "TFM_RTL_VARIANT must be set to either fpga or emu\n")
    endif()

else()
      message(FATAL_ERROR "Please set TFM_PLATFORM_VARIANT\n")
endif()

#========================= Platform Region Defs ===============================#

target_include_directories(platform_region_defs
    INTERFACE
        .
)

#========================= Platform BL1_1 =====================================#

target_sources(platform_bl1_1
    PRIVATE
        $<IF:$<BOOL:${PLATFORM_HAS_STRATA_FLASH}>,
            cmsis_drivers/Driver_Flash.c,
            cmsis_drivers/Driver_Simple_Memory_Flash.c>
        device/rse_expansion_device_definition.c
        device/host_device_definition.c
        $<$<BOOL:${PLATFORM_HAS_STRATA_FLASH}>:${PLATFORM_DIR}/ext/target/arm/drivers/flash/cfi/cfi_drv.c>
        $<$<BOOL:${PLATFORM_HAS_STRATA_FLASH}>:${PLATFORM_DIR}/ext/target/arm/drivers/flash/strata/spi_strataflashj3_flash_lib.c>
        $<$<BOOL:${RSE_USE_HOST_UART}>:${PLATFORM_DIR}/ext/target/arm/drivers/usart/pl011/uart_pl011_drv.c>
        $<$<BOOL:${RSE_USE_HOST_UART}>:${CMAKE_CURRENT_SOURCE_DIR}/cmsis_drivers/Driver_USART_pl011.c>
)

target_compile_definitions(platform_bl1_1
    PRIVATE
        $<$<BOOL:${PLATFORM_HAS_STRATA_FLASH}>:PLATFORM_HAS_STRATA_FLASH>
)

target_include_directories(platform_bl1_1_interface
    INTERFACE
        cmsis_drivers
        device
        $<$<BOOL:${PLATFORM_HAS_STRATA_FLASH}>:${PLATFORM_DIR}/ext/target/arm/drivers/flash/cfi>
        $<$<BOOL:${PLATFORM_HAS_STRATA_FLASH}>:${PLATFORM_DIR}/ext/target/arm/drivers/flash/common>
        $<$<BOOL:${PLATFORM_HAS_STRATA_FLASH}>:${PLATFORM_DIR}/ext/target/arm/drivers/flash/strata>
        $<$<BOOL:${RSE_USE_HOST_UART}>:${PLATFORM_DIR}/ext/target/arm/drivers/usart/pl011>
)

#========================= Platform BL1_2 =====================================#

target_include_directories(platform_bl1_2
    PRIVATE
        $<$<BOOL:${TFM_PARTITION_FIRMWARE_UPDATE}>:${CMAKE_CURRENT_SOURCE_DIR}/fwu>
    PUBLIC
        cmsis_drivers
        device
        $<$<BOOL:${PLATFORM_HAS_STRATA_FLASH}>:${PLATFORM_DIR}/ext/target/arm/drivers/flash/cfi>
        $<$<BOOL:${PLATFORM_HAS_STRATA_FLASH}>:${PLATFORM_DIR}/ext/target/arm/drivers/flash/common>
        $<$<BOOL:${PLATFORM_HAS_STRATA_FLASH}>:${PLATFORM_DIR}/ext/target/arm/drivers/flash/strata>
        $<$<BOOL:${RSE_USE_HOST_UART}>:${PLATFORM_DIR}/ext/target/arm/drivers/usart/pl011>
)

target_sources(platform_bl1_2
    PUBLIC
        $<$<BOOL:${TFM_PARTITION_FIRMWARE_UPDATE}>:${CMAKE_CURRENT_SOURCE_DIR}/fwu/fwu_hal_bl1_2.c>
        $<$<BOOL:${TFM_PARTITION_FIRMWARE_UPDATE}>:${CMAKE_CURRENT_SOURCE_DIR}/fwu/fwu_private_metadata.c>
)

#========================= Platform BL2 =======================================#

target_sources(platform_bl2
    PRIVATE
        bl2/boot_hal_bl2.c
        bl2/flash_map_bl2.c
        bl2/scmi_comms.c
        $<IF:$<BOOL:${PLATFORM_HAS_STRATA_FLASH}>,
            cmsis_drivers/Driver_Flash.c,
            cmsis_drivers/Driver_Simple_Memory_Flash.c>
        device/host_device_definition.c
        device/rse_expansion_device_definition.c
        device/tfm_plat_boot_measurement.c
        $<$<BOOL:${PLATFORM_HOST_HAS_SI_CL}>:${CMAKE_CURRENT_SOURCE_DIR}/host_drivers/ppu_drv.c>
        $<$<BOOL:${PLATFORM_HOST_HAS_SI_CL}>:${CMAKE_CURRENT_SOURCE_DIR}/host_drivers/scr_drv.c>
        scmi/scmi_hal.c
        scmi/scmi_power_domain.c
        $<$<BOOL:${PLATFORM_HAS_STRATA_FLASH}>:${PLATFORM_DIR}/ext/target/arm/drivers/flash/cfi/cfi_drv.c>
        $<$<BOOL:${PLATFORM_HAS_STRATA_FLASH}>:${PLATFORM_DIR}/ext/target/arm/drivers/flash/strata/spi_strataflashj3_flash_lib.c>
        ${PLATFORM_DIR}/ext/target/arm/rse/common/fip_parser/fip_parser.c
        $<$<BOOL:${RSE_USE_HOST_UART}>:${PLATFORM_DIR}/ext/target/arm/drivers/usart/pl011/uart_pl011_drv.c>
        $<$<BOOL:${RSE_USE_HOST_UART}>:${CMAKE_CURRENT_SOURCE_DIR}/cmsis_drivers/Driver_USART_pl011.c>
        $<$<BOOL:${TFM_PARTITION_FIRMWARE_UPDATE}>:${CMAKE_CURRENT_SOURCE_DIR}/fwu/fwu_hal_bl2.c>
        $<$<BOOL:${TFM_PARTITION_FIRMWARE_UPDATE}>:${CMAKE_CURRENT_SOURCE_DIR}/fwu/fwu_private_metadata.c>
)

target_include_directories(platform_bl2
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/fwu
    PUBLIC
        bl2
        cmsis_drivers
        device
        host_drivers
        scmi
        $<$<BOOL:${PLATFORM_HAS_STRATA_FLASH}>:${PLATFORM_DIR}/ext/target/arm/drivers/flash/cfi>
        $<$<BOOL:${PLATFORM_HAS_STRATA_FLASH}>:${PLATFORM_DIR}/ext/target/arm/drivers/flash/common>
        $<$<BOOL:${PLATFORM_HAS_STRATA_FLASH}>:${PLATFORM_DIR}/ext/target/arm/drivers/flash/strata>
        ${PLATFORM_DIR}/ext/target/arm/rse/common/fip_parser
        $<$<BOOL:${RSE_USE_HOST_UART}>:${PLATFORM_DIR}/ext/target/arm/drivers/usart/pl011>
)

target_compile_definitions(platform_bl2
    PRIVATE
        $<$<BOOL:${PLATFORM_HOST_HAS_SI_CL}>:PLATFORM_HOST_HAS_SI_CL>
        $<$<BOOL:${PLATFORM_RSE_HAS_ATU_OWNERSHIP}>:PLATFORM_RSE_HAS_ATU_OWNERSHIP>
        SCMI_COMMS_FOR_BL2_POLLING_MODE
        $<$<BOOL:${PLATFORM_HAS_STRATA_FLASH}>:PLATFORM_HAS_STRATA_FLASH>
)

target_sources(bootutil
    PRIVATE
        bl2/rse_bl2_rotkw.c
)

#========================= Aspen runtime image loader =========================#

add_library(runtime_image_loader STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/bootloader/rse_image_loader/src/rse_image_load_util.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bootloader/rse_image_loader/src/tlv.c
    ${CMAKE_SOURCE_DIR}/bl2/src/flash_map.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bl2/flash_map_bl2.c
    ${CMAKE_SOURCE_DIR}/platform/ext/common/boot_hal_bl2.c
    $<$<BOOL:${TFM_PARTITION_FIRMWARE_UPDATE}>:${CMAKE_CURRENT_SOURCE_DIR}/fwu/fwu_hal_bl2.c>
    $<$<BOOL:${TFM_PARTITION_FIRMWARE_UPDATE}>:${CMAKE_CURRENT_SOURCE_DIR}/fwu/fwu_private_metadata.c>
)

target_include_directories(runtime_image_loader
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/fwu
        $<TARGET_PROPERTY:bootutil,INCLUDE_DIRECTORIES>
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/bootloader/rse_image_loader/include
)

target_link_libraries(runtime_image_loader
    PRIVATE
        bl2_crypto_config
        tfm_sprt
)

target_compile_definitions(runtime_image_loader
    PRIVATE
        $<$<BOOL:${PLATFORM_RSE_HAS_ATU_OWNERSHIP}>:PLATFORM_RSE_HAS_ATU_OWNERSHIP>
        $<$<BOOL:${TFM_RUNTIME_DECRYPTION}>:TFM_RUNTIME_DECRYPTION>
)

#========================= Platform Secure ====================================#

target_include_directories(platform_s
    PUBLIC
        cmsis_drivers
        device
        $<$<BOOL:${PLATFORM_HAS_STRATA_FLASH}>:${PLATFORM_DIR}/ext/target/arm/drivers/flash/cfi>
        $<$<BOOL:${PLATFORM_HAS_STRATA_FLASH}>:${PLATFORM_DIR}/ext/target/arm/drivers/flash/common>
        $<$<BOOL:${PLATFORM_HAS_STRATA_FLASH}>:${PLATFORM_DIR}/ext/target/arm/drivers/flash/strata>
        $<$<BOOL:${RSE_USE_HOST_UART}>:${PLATFORM_DIR}/ext/target/arm/drivers/usart/pl011>
)

target_sources(platform_s
    PRIVATE
        $<IF:$<BOOL:${PLATFORM_HAS_STRATA_FLASH}>,
            cmsis_drivers/Driver_Flash.c,
            cmsis_drivers/Driver_Simple_Memory_Flash.c>
        device/host_device_definition.c
        device/rse_expansion_device_definition.c
        $<$<BOOL:${TFM_MULTI_CORE_TOPOLOGY}>:${CMAKE_CURRENT_SOURCE_DIR}/sfcp/sfcp_platform.c>
        $<$<BOOL:${TFM_MULTI_CORE_TOPOLOGY}>:${CMAKE_CURRENT_SOURCE_DIR}/sfcp/sfcp_permissions_hal.c>
        $<$<BOOL:${PLATFORM_HAS_STRATA_FLASH}>:${PLATFORM_DIR}/ext/target/arm/drivers/flash/cfi/cfi_drv.c>
        $<$<BOOL:${PLATFORM_HAS_STRATA_FLASH}>:${PLATFORM_DIR}/ext/target/arm/drivers/flash/strata/spi_strataflashj3_flash_lib.c>
        $<$<BOOL:${RSE_USE_HOST_UART}>:${PLATFORM_DIR}/ext/target/arm/drivers/usart/pl011/uart_pl011_drv.c>
        $<$<BOOL:${RSE_USE_HOST_UART}>:${CMAKE_CURRENT_SOURCE_DIR}/cmsis_drivers/Driver_USART_pl011.c>
)

target_compile_definitions(platform_s
    PUBLIC
        SCMI_COMMS_FOR_RUNTIME_IRQ_NOTIFICATIONS
        SFCP_ATU_REGION_SIZE=${SFCP_ATU_REGION_SIZE}
        $<$<BOOL:${PLATFORM_RSE_HAS_ATU_OWNERSHIP}>:PLATFORM_RSE_HAS_ATU_OWNERSHIP>
)

target_compile_definitions(platform_s
    PRIVATE
        $<$<BOOL:${PLATFORM_HAS_STRATA_FLASH}>:PLATFORM_HAS_STRATA_FLASH>
)

#========================= SCMI Hardware Abstraction Layer ====================#

add_library(scmi_hal INTERFACE)

target_include_directories(scmi_hal
    INTERFACE
        scmi
)

target_sources(scmi_hal
    INTERFACE
        scmi/scmi_hal.c
)

target_link_libraries(scmi_hal
    INTERFACE
        runtime_image_loader
)
