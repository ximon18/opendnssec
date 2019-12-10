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
#include "daemon/signertasks.h"


void setup_mock_worker(e2e_test_state_type *state)
{
    // prepare an "engine"
    state->context = calloc(1, sizeof(struct worker_context));

    state->context->engine = engine_create();
    state->context->worker = worker_create(strdup("MOCK_WORKER"),
        state->context->engine->taskq);
    state->context->worker->context = state->context;
    state->context->worker->need_to_exit = 0;
    state->context->engine->config = calloc(1, sizeof(engineconfig_type));
    state->context->engine->config->num_signer_threads = 0;

    // build a minimal zone
    // create a tmeporary copy of the zone name because zone_create edits it
    // _before_ it copies it !!!
    char* tmp_zone_name = strdup(MOCK_ZONE_NAME);
    zone_type* zone = zone_create(tmp_zone_name, LDNS_RR_CLASS_IN);
    free(tmp_zone_name);

    zone->signconf->sig_resign_interval = duration_create_from_string("P2D");
    zone->signconf->sig_refresh_interval = duration_create_from_string("P7D");
    zone->signconf->sig_validity_default = duration_create_from_string("P14D");
    zone->signconf->sig_validity_denial = duration_create_from_string("P14D");
    zone->signconf->keys = keylist_create(zone->signconf);
    zone->signconf->nsec_type = LDNS_RR_TYPE_NSEC;
    zone->adoutbound = adapter_create("MOCK_CONFIG_STR", -1, 0);
    state->zone = zone;

    // create a ZSK and KSK for signing
    e2e_prepare_for_keys(2);
    e2e_init_mock_key("MOCK_KSK_LOCATOR", zone, true, false);
    e2e_init_mock_key("MOCK_ZSK_LOCATOR", zone, false, true);
}

void teardown_mock_worker(e2e_test_state_type *state)
{
    assert_non_null(state);
    zone_cleanup(state->zone);
    worker_cleanup(state->context->worker);
    engine_cleanup(state->context->engine);
    free(state->context);
}

zone_type * configure_mock_worker(
    e2e_test_state_type* state,
    const char *input_zone)
{
    if (input_zone)
    {
        zone_type *zone = state->zone;
        zone->signconf_filename = strdup(MOCK_ZONE_SIGNCONF_NAME);
        zone->adinbound = adapter_create(MOCK_ZONE_FILE_NAME, ADAPTER_FILE, 1);
        set_mock_input_zone_file(input_zone);

        zone_start(zone);

        schedule_scheduletask(
            state->context->engine->taskq, TASK_READ, zone->name, zone, &zone->zone_lock, schedule_PROMPTLY);
    }

    return state->zone;
}


// ----------------------------------------------------------------------------
// Weak implementation overrides.
// These replace existing definitions whose implementation marked the method
// with __attribute__((weak)) thereby allowing us to override the definition.
// ----------------------------------------------------------------------------

// Mock worker task queuing and drudger thread
void worker_queue_domain(struct worker_context* context, fifoq_type* q, void* item, long* nsubtasks)
{
    // emulate a drudger thread picking up a queued task, don't actually queue
    // anything instead just execute the drudger action that is performed on
    // dequeued tasks, i.e. sign an rrset.
    MOCK_ANNOUNCE();
    hsm_ctx_t *ctx = mock();
    assert_int_equal(ODS_STATUS_OK, signdomain(context, ctx, (recordset_type)item));
}


// ----------------------------------------------------------------------------
// monkey patches:
// these require compilation with -Wl,--wrap=ods_log_debug,--wrap=xxx etc to 
// cause these wrapper implementations to replace the originals.
// ----------------------------------------------------------------------------
ods_status __wrap_schedule_task(schedule_type* schedule, task_type* task, int replace, int log)
{
    TEST_LOG("mock") "mock: %s() owner=%s class=%s type=%s\n", __func__, task->owner, task->class, task->type);
    will_return(__wrap_schedule_pop_task, task);
    return ODS_STATUS_OK;
}

task_type* __wrap_schedule_pop_task(schedule_type* schedule)
{
    MOCK_ANNOUNCE();
    return mock();
}

void __wrap_schedule_unscheduletask(schedule_type* schedule, task_id type, const char* owner)
{
    MOCK_ANNOUNCE();
}

void __wrap_task_perform(schedule_type* scheduler, task_type* task, void* context)
{
    static int sign_count = 0;

    MOCK_ANNOUNCE();

    task_id expected_task_type = mock();

    if (expected_task_type == TASK_STOP) {
        TEST_LOG("mock") "Test end requested, ignoring scheduled task %s.\n", task->type);
        worker_type* worker = mock();
        worker->need_to_exit = 1;
        return NULL;
    }

    if (expected_task_type[0] >= '0' && expected_task_type[0] <= '9') {
        TEST_LOG("mock") "Time change requested, setting time to %s.\n", expected_task_type);
        set_mock_time_now_from_str(expected_task_type);
        expected_task_type = mock();
    }

    assert_string_equal(expected_task_type, task->type);

    if (expected_task_type == TASK_SIGN) sign_count++;
    if (sign_count == 2) {
        // e2e_test_state_type *c = (e2e_test_state_type *) context;
        struct worker_context *c = (struct worker_context *) context;
        zone_type *z = c->zone;
        z->nextserial = malloc(sizeof(uint32_t));
        *(z->nextserial) = 2019010101;
    } else if (sign_count > 2) {
        struct worker_context *c = (struct worker_context *) context;
        zone_type *z = c->zone;
        if (z->nextserial) {
            free(z->nextserial);
            z->nextserial = 0;
        }
    }

    ignore_function_calls(mock_C_Sign);
    ignore_function_calls(__wrap_adapter_write);

    time_t the_time_now = time_now();
    char time_now_str[20];
    strftime(time_now_str, 20, "%Y-%m-%d %H:%M:%S", localtime(&the_time_now));
    TEST_LOG("mock") "Performing task %s at %d (%s).\n", task->type, the_time_now, time_now_str);

    __real_task_perform(scheduler, task, context);
}
int __wrap_metastorageget(const char* name, void* item)
{
    MOCK_ANNOUNCE();
    return 0;
}
int __wrap_metastorageput(void* item)
{
    MOCK_ANNOUNCE();
}
