/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: Copyright The TrustedFirmware-M Contributors
 *
 */

#ifndef __SCMI_HAL_H__
#define __SCMI_HAL_H__

#include "bl2/scmi_comms.h"
#include "scmi_hal_defs.h"
#include "scmi_log.h"
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SCMI_COMMS_FOR_BL2_POLLING_MODE
#define SCMI_HAL_WAIT_TIME 1000000
#endif /* SCMI_COMMS_FOR_BL2_POLLING_MODE */

#ifdef SCMI_COMMS_FOR_RUNTIME_IRQ_NOTIFICATIONS
/**
 * \brief Designations for different pages of the SRAM block. Page
 *        numbers are in consecutive order.
 */
enum scmi_sram_page_designations {
    SCMI_COMMAND_SRAM_PAGE,
    SCMI_NOTIFIER_SRAM_PAGE,
    SCMI_SRAM_PAGE_COUNT
};

/**
 * \brief Structure for associating SRAM region and MHU doorbell flag bit with
 *        SCMI message type.
 */
struct scmi_sram_page_config {
    struct transport_buffer_t *const base_addr; /* Transport buffer used to hold messages */
    uint32_t pbx_db_flag_bit; /* Associated postbox (outgoing) doorbell flag */
    uint32_t mbx_db_flag_bit; /* Associated mailbox (incoming) doorbell flag */
    uint32_t page_access;     /* Page is available for safe use */
};
#endif /* SCMI_COMMS_FOR_RUNTIME_IRQ_NOTIFICATIONS */

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

#ifdef SCMI_COMMS_FOR_BL2_POLLING_MODE
/**
 * \brief busy wait for a period (time depends on the instruction cycle)
 *
 * \param[in] cycles
 */
void scmi_hal_wait(uint32_t cycles);
#endif /* SCMI_COMMS_FOR_BL2_POLLING_MODE */

#ifdef SCMI_COMMS_FOR_RUNTIME_IRQ_NOTIFICATIONS
/**
 * \brief Read from shared memory at the page corresponding to the doorbell
 *        flag bit set.
 *
 * \param[in] db_flag_bit  Doorbell flag bit to select which shared memory page to read.
 * \param[out] msg  SCMI message buffer for reading a message from shared memory.
 *
 *  \return SCMI_COMMS_SUCCESS on success, SCMI_COMMS_GENERIC_ERROR or SCMI_COMMS_INVALID_ARGUMENT
 *          on failure.
 */
scmi_comms_err_t scmi_hal_shared_memory_read(uint32_t db_flag_bit, struct scmi_message_t *msg);

/**
 * \brief Handle a system power state change.
 *
 * \param[in] agent_id      Identifier of the agent that caused the power state change
 * \param[in] flags         Power state change flags
 * \param[in] system_state  Power state that is being transitioned to
 *
 * \return SCMI status value.
 */
int32_t scmi_hal_sys_power_state(uint32_t agent_id, uint32_t flags,
                                 uint32_t system_state);
#endif /* SCMI_COMMS_FOR_RUNTIME_IRQ_NOTIFICATIONS */
#ifdef __cplusplus
}
#endif

#endif /* __SCMI_HAL_H__ */
