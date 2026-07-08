/*
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __FWU_ESRT_H__
#define __FWU_ESRT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "fwu_private_metadata.h"
#include "psa/error.h"
#include "psa/update.h"

/* Firmware Type Definitions */
#define ESRT_FW_TYPE_UNKNOWN                                0x00000000
#define ESRT_FW_TYPE_SYSTEMFIRMWARE                         0x00000001
#define ESRT_FW_TYPE_DEVICEFIRMWARE                         0x00000002
#define ESRT_FW_TYPE_UEFIDRIVER                             0x00000003

/* Last Attempt Status Values */
#define LAST_ATTEMPT_STATUS_SUCCESS                         0x00000000
#define LAST_ATTEMPT_STATUS_ERROR_UNSUCCESSFUL              0x00000001
#define LAST_ATTEMPT_STATUS_ERROR_INSUFFICIENT_RESOURCES    0x00000002
#define LAST_ATTEMPT_STATUS_ERROR_INCORRECT_VERSION         0x00000003
#define LAST_ATTEMPT_STATUS_ERROR_INVALID_FORMAT            0x00000004
#define LAST_ATTEMPT_STATUS_ERROR_AUTH_ERROR                0x00000005
#define LAST_ATTEMPT_STATUS_ERROR_PWR_EVT_AC                0x00000006
#define LAST_ATTEMPT_STATUS_ERROR_PWR_EVT_BATT              0x00000007
#define LAST_ATTEMPT_STATUS_ERROR_UNSATISFIED_DEPENDENCIES  0x00000008

/* Convert the image version into uint32_t type. */
#define CONVERT_FWU_VERSION(major, minor, revision)    \
                            ((uint32_t)((((uint32_t)(major) & 0xFF) << 24) | \
                                        (((uint32_t)(minor) & 0xFF) << 16) | \
                                        (revision & 0xFFFF)))

/**
 * \brief                   Get the version of the active firmware image
 *                          identified by \p component.
 *
 * \param[in]  mdata        \ref fwu_private_metadata.
 * \param[in]  component    \ref psa_fwu_component_t of the firmware image.
 *
 * \param[out] version      The \p psa_fwu_image_version_t of the firmware
 *                          image.
 *
 * \return                  \ref PSA_SUCCESS if operation completed
 *                          successfully. Otherwise another value on error.
 */
psa_status_t esrt_get_active_image_version(struct fwu_private_metadata *mdata,
                                           psa_fwu_component_t component,
                                           psa_fwu_image_version_t *fwu_version);

/**
 * \brief                   Update Last Attempt Version and Last Attempt Status
 *                          of the firmware image identified by \p component.
 *
 * \param[in]  mdata        \ref fwu_private_metadata.
 * \param[in]  component    \ref psa_fwu_component_t of the firmware image.
 *
 * \return                  \ref PSA_SUCCESS if operation completed
 *                          successfully. Otherwise another value on error.
 */
psa_status_t esrt_update_last_attempt(struct fwu_private_metadata *mdata,
                                      psa_fwu_component_t component);

/**
 * \brief                   Initialize boot data for ESRT
 *
 * \return                  \ref PSA_SUCCESS if operation completed
 *                          successfully. Otherwise another value on error.
 */
psa_status_t esrt_init_boot_data(void);

#ifdef __cplusplus
}
#endif

#endif /* __FWU_ESRT_H__ */
