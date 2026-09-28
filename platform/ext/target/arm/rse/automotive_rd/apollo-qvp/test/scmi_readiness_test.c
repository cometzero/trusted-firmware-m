/* SPDX-License-Identifier: BSD-3-Clause */
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void test_memory_barrier(void);
#define __sync_synchronize test_memory_barrier
#include "../bl2/scmi_comms.c"
#include "../scmi/scmi_power_domain.c"
#undef __sync_synchronize

uint64_t test_mailbox[32];
static uint64_t clock_ticks;
static uint32_t frequency, doorbell, sends, polls;
static int scenario;
static unsigned int busy_injections, waits;

static void test_memory_barrier(void)
{
    if ((scenario == 10 || scenario == 11) && busy_injections == 0) {
        /* Between the outer FREE observation and try_send's ownership check. */
        shared_memory->status = 0;
        ++busy_injections;
    }
}

scmi_comms_err_t scmi_hal_doorbell_init(void) { return SCMI_COMMS_SUCCESS; }
scmi_comms_err_t scmi_hal_shared_memory_init(void) { return SCMI_COMMS_SUCCESS; }
scmi_comms_err_t scmi_hal_doorbell_clear(void)
{
    doorbell = 0;
    return SCMI_COMMS_SUCCESS;
}
scmi_comms_err_t scmi_hal_doorbell_ring(void)
{
    assert(!(shared_memory->status & TRANSPORT_BUFFER_STATUS_FREE_MASK));
    assert(shared_memory->length == sizeof(shared_memory->message_header));
    assert(shared_memory->flags & TRANSPORT_BUFFER_FLAGS_INTERRUPT_MASK);
    ++sends;
    return SCMI_COMMS_SUCCESS;
}
void scmi_hal_wait(uint32_t cycles) { (void)cycles; ++waits; }
uint64_t scmi_hal_counter_ticks(void) { return clock_ticks++; }
uint32_t scmi_hal_counter_frequency(void) { return frequency; }

static void response(void)
{
    shared_memory->status = TRANSPORT_BUFFER_STATUS_FREE_MASK;
    shared_memory->length = 12;
    shared_memory->message_payload[0] = 0;
    shared_memory->message_payload[1] = 0x20000;
}

scmi_comms_err_t scmi_hal_doorbell_read(uint32_t *value)
{
    ++polls;
    if (scenario == 10 && polls == 3) {
        assert(sends == 0 && shared_memory->length == 0);
        shared_memory->status = TRANSPORT_BUFFER_STATUS_FREE_MASK;
    }
    if (scenario == 2 && polls == 4) {
        /* SCP completer initialization discards an early startup request. */
        memset(test_mailbox, 0, sizeof(test_mailbox));
        shared_memory->status = TRANSPORT_BUFFER_STATUS_FREE_MASK;
    }
    if (scenario != 3 && scenario != 7 && scenario != 11 && polls == 8) {
        response();
        if (scenario == 4)
            shared_memory->message_header ^= 1;
        if (scenario == 5)
            shared_memory->length = 0;
        if (scenario == 6)
            shared_memory->message_payload[0] = SCMI_STATUS_DENIED;
        if (scenario == 8)
            clock_ticks = 101;
        if (scenario == 9)
            shared_memory->status |= TRANSPORT_BUFFER_STATUS_ERROR_MASK;
        /* Response data/ownership may precede the doorbell. */
        if (scenario != 1)
            doorbell = SI_MHU_COMMAND_MBX_FLAG;
    }
    if (scenario == 1 && polls == 12)
        doorbell = SI_MHU_COMMAND_MBX_FLAG;
    if (scenario == 7 && polls == 10) {
        response();
        doorbell = SI_MHU_COMMAND_MBX_FLAG;
    }
    *value = doorbell;
    return SCMI_COMMS_SUCCESS;
}

static void reset(int mode)
{
    memset(test_mailbox, 0, sizeof(test_mailbox));
    shared_memory->status = TRANSPORT_BUFFER_STATUS_FREE_MASK;
    frequency = 10;
    clock_ticks = doorbell = sends = polls = 0;
    busy_injections = waits = 0;
    scenario = mode;
}

int main(void)
{
    uint32_t version = 0;
    struct scmi_message_t msg = { .header = 0x4400 };

    reset(0);
    assert(scmi_comm_get_power_domain_version(&version) == SCMI_COMMS_SUCCESS);
    assert(version == 0x20000 && sends == 1 && polls == 8);
    reset(1);
    assert(scmi_comm_get_power_domain_version(&version) == SCMI_COMMS_SUCCESS);
    assert(sends == 1 && polls == 12);
    reset(2);
    assert(scmi_comm_get_power_domain_version(&version) == SCMI_COMMS_SUCCESS);
    assert(sends == 2);
    reset(3);
    assert(scmi_comm_get_power_domain_version(&version) != SCMI_COMMS_SUCCESS);
    assert(sends == 1 && polls > 10 && polls <= 99);
    reset(4);
    assert(scmi_comm_get_power_domain_version(&version) == SCMI_COMMS_INVALID_ARGUMENT);
    reset(5);
    assert(scmi_comm_get_power_domain_version(&version) == SCMI_COMMS_INVALID_ARGUMENT);
    reset(6);
    assert((int)scmi_comm_get_power_domain_version(&version) == SCMI_STATUS_DENIED);
    reset(7);
    frequency = 1;
    assert(scmi_comm_get_power_domain_version(&version) != SCMI_COMMS_SUCCESS);
    assert(polls <= 9 && sends == 1);
    reset(0);
    frequency = 0;
    assert(transport_exchange(&msg, 10) == SCMI_COMMS_INVALID_ARGUMENT);
    assert(sends == 0);
    reset(0);
    assert(transport_exchange(&msg, 0) == SCMI_COMMS_INVALID_ARGUMENT);
    assert(transport_exchange(NULL, 10) == SCMI_COMMS_INVALID_ARGUMENT);
    reset(0);
    clock_ticks = UINT64_MAX - 3;
    assert(scmi_comm_get_power_domain_version(&version) == SCMI_COMMS_SUCCESS);
    assert(sends == 1);
    reset(8);
    assert(scmi_comm_get_power_domain_version(&version) != SCMI_COMMS_SUCCESS);
    reset(9);
    assert(scmi_comm_get_power_domain_version(&version) != SCMI_COMMS_SUCCESS);
    reset(10);
    assert(scmi_comm_get_power_domain_version(&version) == SCMI_COMMS_SUCCESS);
    assert(busy_injections == 1 && sends == 1 && waits == 0);
    reset(11);
    assert(scmi_comm_get_power_domain_version(&version) != SCMI_COMMS_SUCCESS);
    assert(busy_injections == 1 && sends == 0 && waits == 0);
    assert(polls > 10 && polls <= 99);
    assert(shared_memory->length == 0);
    reset(0);
    shared_memory->status = 0;
    assert(transport_send(&msg) == SCMI_STATUS_GENERIC_ERROR);
    assert(waits == 1 && sends == 0);
    reset(0);
    assert(transport_send(&msg) == SCMI_COMMS_SUCCESS);
    assert(waits == 1 && sends == 1);
    puts("PASS: 17 SCMI startup readiness cases");
    return 0;
}
