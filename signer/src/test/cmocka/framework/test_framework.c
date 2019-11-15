/*
 * Copyright (c) 2009 NLNet Labs. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *    notice, this list
  of conditions and the following disclaimer.
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
#include "mock_hsm.h"
#include "mock_worker.h"


#include <pkcs11.h>
#include <stdarg.h>


#include "daemon/engine.h"


static test_keys_type *e2e_mock_keys      = NULL;
static int             e2e_mock_key_count = 0;


void e2e_prepare_for_keys(int num_keys)
{
    e2e_mock_keys = calloc(2, sizeof(test_keys_type));
}

test_keys_type * e2e_get_mock_keys(void)
{
    return e2e_mock_keys;
}

int e2e_get_mock_key_count(void)
{
    return e2e_mock_key_count;
}

void e2e_init_mock_key(
    const char *locator, const zone_type *zone, bool is_ksk, bool is_zsk)
{
    int new_key_idx = e2e_mock_key_count++;
    test_keys_type *mock_key = &e2e_mock_keys[new_key_idx];

    mock_key->ldns_key = ldns_key_new_frm_algorithm(LDNS_RSASHA256, 2048);
    mock_key->ldns_rr  = ldns_key2rr(mock_key->ldns_key);
    mock_key->locator  = strdup(locator);
    mock_key->hsm_key  = calloc(1, sizeof(libhsm_key_t));

    ldns_rr_set_owner(mock_key->ldns_rr, ldns_rdf_clone(zone->apex));

    mock_key->hsm_key->modulename = strdup(MOCK_HSM_MODULE_NAME);
    mock_key->hsm_key->private_key = new_key_idx;

    keylist_push(
        zone->signconf->keys,
        mock_key->locator,
        NULL,
        mock_key->ldns_key->_alg,
        LDNS_KEY_ZONE_KEY,
        1,      // publish this key
        is_ksk,
        is_zsk);
}

int e2e_setup(void** cmocka_state)
{
    e2e_test_state_type *state = calloc(1, sizeof(e2e_test_state_type));
    state->hsm_ctx = setup_mock_hsm();
    setup_mock_worker(state);
    *cmocka_state = state;
    return 0;
}

int e2e_teardown(e2e_test_state_type** state)
{
    e2e_test_state_type *e2e_test_state = *state;
    teardown_mock_worker(e2e_test_state);
    teardown_mock_hsm(e2e_test_state->hsm_ctx);
    for (int i = 0; i < e2e_mock_key_count; i++) {
        test_keys_type *mock_key = &e2e_mock_keys[i];
        free(mock_key->hsm_key->modulename);
        free(mock_key->hsm_key);
        free(mock_key->locator);
        ldns_rr_free(mock_key->ldns_rr);
        ldns_key_free(mock_key->ldns_key);
    }
    free(e2e_mock_keys);
    free(e2e_test_state);
    e2e_mock_key_count = 0;
    return 0;
}

void e2e_configure_mocks(
    const e2e_test_state_type* state,
    const task_id task_id,
    const char *input_zone)
{
    // state->worker->task->what = task_id;
    configure_mock_hsm(state);
    configure_mock_worker(state, input_zone);
}

void e2e_go(const e2e_test_state_type* state, ...)
{
    va_list args;

    va_start(args, state);
    task_id task_type;
    while (TASK_STOP != (task_type = va_arg(args, task_id))) {
        will_return(__wrap_task_perform, task_type);
    }
    va_end(args);

    // inject the poision pill so that the last task can be detected.
    // inject the worker so that it can be signalled to exit.
    will_return(__wrap_task_perform, TASK_STOP);
    will_return(__wrap_task_perform, state->context->worker);

    worker_start(state->context->worker);
}

int unittest_setup(void **state)
{
    char* zone_name = strdup("test_zone");
    zone_type *zone = zone_create(zone_name, LDNS_RR_CLASS_IN);
    free(zone_name);
    (*state) = zone;
    return 0;
}

int unittest_teardown(void **state)
{
    zone_type *zone = (zone_type *)(*state);
    zone_cleanup(zone);
    return 0;
}