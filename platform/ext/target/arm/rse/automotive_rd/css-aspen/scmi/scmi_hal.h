/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 *
 */

#ifndef __SCMI_HAL_H__
#define __SCMI_HAL_H__

#include "scmi_comms.h"
#include "scmi_hal_defs.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCMI_HAL_WAIT_TIME 1000000

/**
 * \brief Initialize the SCMI transport shared memory area.
 *
 * \return SCMI_COMMS_SUCCESS on success, SCMI_COMMS_HARDWARE_ERROR on failure.
 */
scmi_comms_err_t scmi_hal_shared_memory_init(void);

/**
 * \brief Initialize the SCMI transport doorbells.
 *
 * \return SCMI_COMMS_SUCCESS on success, SCMI_COMMS_HARDWARE_ERROR on failure.
 */
scmi_comms_err_t scmi_hal_doorbell_init(void);

/**
 * \brief Ring the SCMI transport doorbell.
 *
 * \return SCMI_COMMS_SUCCESS on success, SCMI_COMMS_HARDWARE_ERROR on failure.
 */
scmi_comms_err_t scmi_hal_doorbell_ring(void);

/**
 * \brief Clear the SCMI transport doorbell.
 *
 * \return SCMI_COMMS_SUCCESS on success, SCMI_COMMS_HARDWARE_ERROR on failure.
 */
scmi_comms_err_t scmi_hal_doorbell_clear(void);

/**
 * \brief Read the SCMI transport doorbell.
 *
 * \return SCMI_COMMS_SUCCESS on success, SCMI_COMMS_HARDWARE_ERROR on failure.
 */
scmi_comms_err_t scmi_hal_doorbell_read(uint32_t *value);

/**
 * \brief busy wait for a period (time depends on the instruction cycle)
 *
 * \param[in] cycles
 */
void scmi_hal_wait(uint32_t cycles);
#ifdef __cplusplus
}
#endif

#endif /* __SCMI_HAL_H__ */
