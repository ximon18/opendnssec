/*
 * Copyright (c) 2009 NLNet Labs. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
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
#ifndef TEST_MOCKS_H
#define TEST_MOCKS_H


// cmocka required includes
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>


#include "scheduler/task.h"
#include "signer/zone.h"
#include "daemon/signertasks.h"
#include "mock_logging.h"
#include "mock_inputs.h"


// ----------------------------------------------------------------------------
// Macros to wrap a test definition so that:
//   - it is clear which functions are tests.
//   - tests register themselves, there is no need to modify the test main
//     function to register them manually.
//   - if needed later post-test logic can be added without modifying the
//     original test code.
// ----------------------------------------------------------------------------
#define E2E_TEST_BEGIN(name) \
    void e2e_test_ ## name(e2e_test_state_type** cmocka_state); \
    __attribute__((constructor)) \
    static void register_e2e_test_ ## name() { \
        struct CMUnitTest test = cmocka_unit_test_setup_teardown(e2e_test_ ## name, e2e_setup, e2e_teardown); \
        register_test(&test); \
    } \
    void e2e_test_ ## name(e2e_test_state_type** cmocka_state) { \
        e2e_test_state_type *state = *cmocka_state;

#define E2E_TEST_END \
    }

#define UNIT_TEST_BEGIN(name) \
    void unit_test_ ## name(void **unused); \
    __attribute__((constructor)) \
    static void register_unit_test_ ## name() { \
        struct CMUnitTest test = cmocka_unit_test(unit_test_ ## name); \
        register_test(&test); \
    } \
    void unit_test_ ## name(void **unused) { \
        
#define UNIT_TEST_END \
    }


// ----------------------------------------------------------------------------
// Types used to pass state into tests.
// ----------------------------------------------------------------------------
typedef struct e2e_test_state_struct {
    struct worker_context *context;
    zone_type   *zone;
    task_type   *task;
    hsm_ctx_t   *hsm_ctx;
} e2e_test_state_type;


typedef struct test_keys_struct {
    ldns_key *  ldns_key;
    ldns_rr *   ldns_rr;
    char *      locator;
    libhsm_key_t * hsm_key;
} test_keys_type;


#define MOCK_ZONE_NAME           "mockzone."
#define MOCK_ASSERT(expression)  mock_assert((int)(expression), #expression, __FILE__, __LINE__);
#define MOCK_POINTER             (void*)0xDEADBEEF
#define TASK_STOP                NULL


test_keys_type* e2e_get_mock_keys(void);
int             e2e_get_mock_key_count(void);
void            e2e_init_mock_key(const char *locator, const zone_type *zone, bool is_ksk, bool is_zsk);
int             e2e_setup(void** state);
int             e2e_teardown(e2e_test_state_type** state);
void            e2e_configure_mocks(const e2e_test_state_type* state, const task_id task_id, const char *input_zone);
void            e2e_go(const e2e_test_state_type* state, ...);


#endif