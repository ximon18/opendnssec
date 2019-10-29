/*
 * Copyright (c) 2009 NLNet Labs. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include "test_framework.h"
#include "mock_worker.h"
#include "mock_logging.h"
#include "mock_hsm.h"
#include "daemon/engine.h"
#include "scheduler/fifoq.h"
#include "signer/rrset.h"
#include "daemon/signertasks.h"


void setup_mock_worker(e2e_test_state_type *state)
{
    // prepare an "engine"
    state->context = calloc(1, sizeof(struct worker_context));
    fprintf(stderr, "XIMON1: %p\n", state);
    fprintf(stderr, "XIMON1: %p\n", state->context);

    // build a minimal zone
    zone_type* zone = zone_create(strdup(MOCK_ZONE_NAME), LDNS_RR_CLASS_IN);
    zone->signconf = signconf_create();
    zone->signconf->sig_resign_interval = duration_create_from_string("P1D");
    zone->signconf->keys = keylist_create(zone->signconf);
    zone->signconf->nsec_type = LDNS_RR_TYPE_NSEC;
    zone->adoutbound = adapter_create("MOCK_CONFIG_STR", -1, 0);
    state->zone = zone;

    // // create a ZSK and KSK for signing
    // e2e_init_mock_key("MOCK_KSK_LOCATOR", zone, true, false);
    // e2e_init_mock_key("MOCK_ZSK_LOCATOR", zone, false, true);

    // return worker;
}

void teardown_mock_worker(e2e_test_state_type *state)
{
    assert_non_null(state);

    zone_type *zone = state->zone;
    if (zone->signconf->soa_serial) free(zone->signconf->soa_serial);
    if (zone->adinbound) {
        if (zone->adinbound->configstr) free(zone->adinbound->configstr);
        free(zone->adinbound);
    }

    free(state->context);
}

void configure_mock_worker(
    e2e_test_state_type* state,
    const char *input_zone)
{
    // will_return_always(worker_queue_rrset, state->hsm_ctx);
    expect_function_call_any(mock_C_Sign);
    expect_function_call(__wrap_adapter_write);

    if (input_zone)
    {
        zone_type *zone = state->zone;
        zone->signconf_filename = "UNUSED MOCK SIGNCONF FILENAME";
        zone->adinbound = calloc(1, sizeof(adapter_type));
        zone->adinbound->type = ADAPTER_FILE;
        zone->adinbound->configstr = strdup("UNUSED MOCK ZONE_FILENAME");
        set_mock_input_zone_file(input_zone);
        // state->task = task_create(
        //     strdup(state->zone->name),
        //     TASK_CLASS_SIGNER,
        //     TASK_READ,
        //     do_readzone,
        //     state->zone, NULL, 0
        // );
        engine_type* engine = engine_create();
        state->context->engine = engine;
        fprintf(stderr, "XIMON2: state=%p\n", state);
        fprintf(stderr, "XIMON2: context=%p\n", state->context);
        fprintf(stderr, "XIMON2: engine1=%p\n", engine);
        fprintf(stderr, "XIMON2: engine2=%p\n", state->context->engine);
        fprintf(stderr, "XIMON2: taskq=%p\n", state->context->engine->taskq);
        schedule_scheduletask(
            state->context->engine->taskq, TASK_READ, zone->name, zone, &zone->zone_lock, schedule_PROMPTLY);
        will_return_always(__wrap_schedule_task, state->context);
    }
}

// ----------------------------------------------------------------------------
// Weak implementation overrides.
// These replace existing definitions whose implementation marked the method
// with __attribute__((weak)) thereby allowing us to override the definition.
// ----------------------------------------------------------------------------

// // Mock worker task queuing and drudger thread
// void worker_queue_rrset(worker_type* worker, fifoq_type* q, rrset_type* rrset) {
//     // emulate a drudger thread picking up a queued task, don't actually queue
//     // anything instead just execute the drudger action that is performed on
//     // dequeued tasks, i.e. sign an rrset.
//     MOCK_ANNOUNCE();
//     hsm_ctx_t *ctx = mock();
//     assert_int_equal(ODS_STATUS_OK, rrset_sign(ctx, rrset, time_now()));
// }


// ----------------------------------------------------------------------------
// monkey patches:
// these require compilation with -Wl,--wrap=ods_log_debug,--wrap=xxx etc to 
// cause these wrapper implementations to replace the originals.
// ----------------------------------------------------------------------------
ods_status
__wrap_schedule_task(schedule_type* schedule, task_type* task, int replace, int log) {
    MOCK_ANNOUNCE();
    // just invoke it directly
    task_perform(schedule, task, mock());
}

void __wrap_schedule_unscheduletask(schedule_type* schedule, task_id type, const char* owner) {
    MOCK_ANNOUNCE();
}

