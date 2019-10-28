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
#include "test_fw.h"
#include "mock_worker.h"
#include "mock_logging.h"
#include "mock_hsm.h"
#include "daemon/engine.h"
#include "daemon/worker.h"
#include "scheduler/fifoq.h"
#include "signer/rrset.h"


worker_type * setup_mock_worker(void)
{
    // build a minimal zone
    zone_type* zone = zone_create(strdup(MOCK_ZONE_NAME), LDNS_RR_CLASS_IN);
    zone->signconf = signconf_create();
    zone->signconf->sig_resign_interval = duration_create_from_string("P1D");
    zone->signconf->keys = keylist_create(zone->signconf);
    zone->signconf->nsec_type = LDNS_RR_TYPE_NSEC;
    zone->adoutbound = adapter_create("MOCK_CONFIG_STR", -1, 0);

    // prepare a task for working on the zone
    // the details of the task are test specific and so not defined here
    task_type* task = calloc(1, sizeof(task_type));
    task->zone = zone;

    // prepare an "engine" - what is an engine actually?
    engine_type* engine = calloc(1, sizeof(engine_type));
    engine->signq = MOCK_POINTER;
    engine->config = MOCK_POINTER;

    // prepare a worker to process the zone task
    worker_type *worker = calloc(1, sizeof(worker_type));
    worker->type = WORKER_WORKER;
    worker->task = task;
    worker->engine = engine;

    // create a ZSK and KSK for signing
    e2e_init_mock_key("MOCK_KSK_LOCATOR", zone, true, false);
    e2e_init_mock_key("MOCK_ZSK_LOCATOR", zone, false, true);

    return worker;
}

void teardown_mock_worker(const worker_type* worker)
{
    assert_non_null(worker);
    // TODO: free ZSK and KSK resources allocated in init above
    free(worker->engine);
    free(worker->task);
    free(worker);
}

void configure_mock_worker(const e2e_test_state_type* state)
{
    will_return_always(worker_queue_rrset, state->hsm_ctx);
    expect_function_call_any(mock_C_Sign);
    expect_function_call(__wrap_adapter_write);
}

// ----------------------------------------------------------------------------
// Weak implementation overrides.
// These replace existing definitions whose implementation marked the method
// with __attribute__((weak)) thereby allowing us to override the definition.
// ----------------------------------------------------------------------------

// Mock worker task queuing and drudger thread
void worker_queue_rrset(worker_type* worker, fifoq_type* q, rrset_type* rrset) {
    // emulate a drudger thread picking up a queued task, don't actually queue
    // anything instead just execute the drudger action that is performed on
    // dequeued tasks, i.e. sign an rrset.
    MOCK_ANNOUNCE();
    hsm_ctx_t *ctx = mock();
    assert_int_equal(ODS_STATUS_OK, rrset_sign(ctx, rrset, time_now()));
}