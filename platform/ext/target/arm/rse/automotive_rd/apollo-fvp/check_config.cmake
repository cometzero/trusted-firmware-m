#-------------------------------------------------------------------------------
# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
#-------------------------------------------------------------------------------

include(${CMAKE_CURRENT_LIST_DIR}/../../common/check_config.cmake)

tfm_invalid_config(MCUBOOT_CUSTOM_DATA_SHARING_FUNCTION AND NOT TFM_PARTITION_MEASURED_BOOT AND NOT TFM_PARTITION_FIRMWARE_UPDATE)
