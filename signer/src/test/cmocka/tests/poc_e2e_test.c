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
#include "test_framework.h"
#include "mock_outputs.h"
#include "mock_worker.h"
#include "mock_core_services.h"
#include "signer/zone.h"
#include <alloca.h>
#include <stdlib.h>
#include <time.h>


const char *ZONE_TEMPLATE = R"(
$TTL	86400 ; 24 hours could have been written as 24h or 1d
; $TTL used for all RRs without explicit TTL value
$ORIGIN <ZONE_NAME> 
@  1D  IN  SOA ns1.example.com. hostmaster.example.com. (
                <ZONE_SERIAL>; serial
                3H ; refresh
                15 ; retry
                1w ; expire
                3h ; nxdomain ttl
                )
    IN  NS     ns1.example.com. ; in the domain
    IN  NS     ns2.smokeyjoe.com. ; external to domain
    IN  MX  10 mail.another.com. ; external mail provider
; server host definitions
ns1    IN  A      192.168.0.1  ;name server definition     
www    IN  A      192.168.0.2  ;web server definition
ftp    IN  CNAME  www.example.com.  ;ftp server definition
; non server domain hosts
bill   IN  A      192.168.0.3
fred   IN  A      192.168.0.4
)";


// Define our own assert_int_equal() because with CMocka() the printed error
// message just indicates the failing line of code, not what the line says and
// thus nothing about what actually failed, at least not at first glance, you
// have to go and read that line of code to understand what was being tested.
void assert_uint32_ptr_equal(const char* field_name, uint32_t *actual, uint32_t *expected)
{
    if (!actual && !expected) return;
    if ((!actual && expected) || (actual && !expected) || (*actual != *expected)) {
        char actual_str[128], expected_str[128];
        if (actual)   snprintf(actual_str, 128, "%d", *actual);
        if (expected) snprintf(expected_str, 128, "%d", *expected);
        fail_msg("%s value %s != %s", field_name,
            actual ? actual_str : "NULL",
            expected ? expected_str : "NULL");
    }
}

// Define a compound checker, because with CMocka it's not possible to register
// multiple checkers which in sequence check the same parameter (in this case
// fields of the same struct parameter ) to the same function.
#define ASSERT_UINT32_PTR_EQUAL(actual, expected) \
    assert_uint32_ptr_equal(#actual, actual, expected)

int check_zone_serials(zone_type *actual, zone_type *expected)
{
    fprintf(stderr, "XIMON: checking expectations..\n");
    assert_string_equal(actual->name, expected->name);
    ASSERT_UINT32_PTR_EQUAL(actual->nextserial,     expected->nextserial);
    ASSERT_UINT32_PTR_EQUAL(actual->inboundserial,  expected->inboundserial);
    // ASSERT_UINT32_PTR_EQUAL(actual->outboundserial, expected->outboundserial);
}

// int get_soa_diff(zone_type *zone)
// {
//     if (!zone->inboundserial || !zone->outboundserial) fail();
//     fprintf(stderr, "XIMON: in=%u out=%u\n", *(zone->inboundserial), *(zone->outboundserial));
//     // return *(zone->outboundserial) - *(zone->inboundserial);
//     return 2;
// }

uint32_t safe_get_outbound_serial(zone_type *zone)
{
    if (!zone->outboundserial) {
        fail();
    } else {
        TEST_LOG("mock") "Inbound serial: %u, outbound serial: %u\n", zone->inboundserial ? *(zone->inboundserial) : 0, *(zone->outboundserial));
        return *(zone->outboundserial);
    }
}

DEFINE_SIGNED_ZONE_CHECK_RETURNS_ZERO(expect_our_zone_is_output, strcmp(zone->name, MOCK_ZONE_NAME));
// DEFINE_SIGNED_ZONE_CHECK_WITH_OP(expect_next_serial, zone->nextserial ? *(zone->nextserial) : fail()), !=);
// DEFINE_SIGNED_ZONE_CHECK_WITH_OP(expect_inbound_serial, zone->inboundserial ? *(zone->inboundserial) : fail()), !=);
DEFINE_SIGNED_ZONE_CHECK_WITH_OP(expect_outbound_serial, safe_get_outbound_serial(zone), !=);
// DEFINE_SIGNED_ZONE_CHECK_WITH_FUNC(expect_output_zone_serials, check_zone_serials);
// DEFINE_SIGNED_ZONE_CHECK_WITH_OP(expect_soa_serial_date_incremented, get_soa_diff(zone), !=);
// DEFINE_SIGNED_ZONE_CHECK_WITH_FUNC(expect_soa_serials, check_zone_serials);

// This cannot be used at present with the develop branch because sig_count is
// never updated. This is a regression compared to the 2.1.x branch.
// DEFINE_SIGNED_ZONE_CHECK(expect_sig_count, zone->stats->sig_count, !=);


int task_perform_cb_set_now(schedule_type* scheduler, task_type* task, void* context, void* cb_data)
{
    TEST_LOG("mock") "Time change requested, setting time to %s.\n", cb_data);
    set_mock_time_now_from_str(cb_data);
    return 0;
}


int task_perform_cb_force_serial(schedule_type* scheduler, task_type* task, void* context, void* cb_data)
{
    TEST_LOG("mock") "Force serial requested, setting next serial to %u.\n", *((uint32_t *)cb_data));
    struct worker_context *c = (struct worker_context *) context;
    zone_type *z = c->zone;
    z->nextserial = cb_data;
    return 0;
}

int task_perform_cb_force_read(schedule_type* scheduler, task_type* task, void* context, void** cb_data)
{
    TEST_LOG("mock") "Force read requested.\n");
    e2e_test_state_type *state = (e2e_test_state_type *) cb_data[0];
    const char *input_zone = (const char *) cb_data[1];
    set_mock_io_buffer(MOCK_ZONE_FILE_NAME, input_zone);
    schedule_scheduletask(
        scheduler, TASK_FORCEREAD, state->zone->name, state->zone, &state->zone->zone_lock, schedule_IMMEDIATELY);
    return 1;
}


#define EXPECT_SERIAL(...)          expect_outbound_serial(atol(vsnprintf_20(__VA_ARGS__)))
#define WILL_LEAP(...)              WILL_INVOKE_CB(task_perform_cb_set_now, STRDUPA(vsnprintf_20(__VA_ARGS__)))
#define WILL_FORCE_SERIAL(serial)   WILL_INVOKE_CB(task_perform_cb_force_serial, SERIALDUP(serial))

// E2E_TEST_BEGIN(force_serial)
//     const char *zone_soa_serial = "20180101";
//     uint32_t zone_soa_serial_num = atol(zone_soa_serial);
//     const char *replacements[] = { MOCK_ZONE_NAME, zone_soa_serial };
//     const char *test_zone = from_template(ZONE_TEMPLATE, replacements);

//     // Given an unsigned input zone
//     zone_type *zone = e2e_configure_mocks(state, test_zone, E2E_VIEWSTATE_NO_RW);

//     // When it is signed using the 'datecounter' soa serial algorithm
//     zone->signconf->soa_serial = strdup("datecounter");
//     zone->signconf->last_modified = 1;

//     // While the worker perform tasks, advance time day by day in January,
//     // such that each sign task occurs on a new day.
//     for (int d = 1; d <= 30; d++) {
//         // arrange for the date that ODS thinks it is today to advance when the
//         // next task is performed.
//         WILL_LEAP("2019-01-%2.2d", d);

//         // if the zone read expectatin has not been set yet, set it now
//         if (d == 1) EXPECT_TASK(TASK_READ);

//         // expect a SIGN task then a WRITE task then when the zone is written
//         // expect that the outbound serial will be the first of 0-99 serial
//         // numbers for the current (mocked) date.
//         EXPECT_TASK(TASK_SIGN);
//         EXPECT_TASK(TASK_WRITE);
//         uint32_t new_serial = atol(vsnprintf_20("201901%2.2d00", d));
//         expect_outbound_serial(new_serial);

//         // advance time by half a day before the next SIGN task and attempt to
//         // force a serial in the past.
//         WILL_LEAP("2019-01-%2.2d 12:00:00", d);
//         uint32_t old_serial = new_serial - 1;
//         WILL_FORCE_SERIAL(old_serial);
//         EXPECT_TASK(TASK_SIGN);
//         EXPECT_TASK(TASK_WRITE);
//         EXPECT_SERIAL("201901%2.2d01", d);
//     }

//     // Run the worker - arrange for it to stop after the tasks define above.
//     e2e_go(state, TASK_STOP);
// E2E_TEST_END


#define YMDH_TO_LEAP_TIME(y, m, d, h) STRDUPA(vsnprintf_20("%4d-%2.2d-%2.2d %2.2d:00:00", y, m, d, h))
#define YMDN_TO_DATECOUNTER(y, m, d, n) STRDUPA(vsnprintf_20("%4d%2.2d%2.2d%2.2d", y, m, d, n))


void datecounter_serial_common(e2e_test_state_type** state, int h_inc)
{
    const char *zone_soa_serial = "20180101"; // some past serial
    uint32_t zone_soa_serial_num = atol(zone_soa_serial);
    const char *replacements[] = { MOCK_ZONE_NAME, zone_soa_serial };
    const char *test_zone = from_template(ZONE_TEMPLATE, replacements);

    // Given an unsigned input zone
    zone_type *zone = e2e_configure_mocks(state, test_zone, E2E_VIEWSTATE_NO_RW);

    // When it is signed using the 'datecounter' soa serial algorithm
    zone->signconf->soa_serial = strdup("datecounter");
    zone->signconf->last_modified = 1;

    // arrange for the date that ODS thinks it is today to advance when the
    // next task is performed.
    int y = 2019, m = 1, d = 1, h = 0, n = 0;
    char *outbound_serial_str = YMDN_TO_DATECOUNTER(y, m, d, n);
    WILL_LEAP(YMDH_TO_LEAP_TIME(y, m, d, h));
    EXPECT_TASK(TASK_READ);
    EXPECT_TASK(TASK_SIGN);
    EXPECT_TASK(TASK_WRITE);
    EXPECT_SERIAL(outbound_serial_str);

    // While the worker perform tasks, advance time day by day in January,
    // such that each sign task occurs on a new day.
    for (d = 2; d <= 31; d++) {
        for (h = 0, n = 0; h < 24; h += h_inc, n++) {
            WILL_LEAP(YMDH_TO_LEAP_TIME(y, m, d, h));

            // force read the unsigned zone again
            char *inbound_serial_str = outbound_serial_str;
            const char *replacements2[] = { MOCK_ZONE_NAME, inbound_serial_str };
            const char *test_zone2 = from_template(ZONE_TEMPLATE, replacements2);
            void *cb_data[] = { state, test_zone2 };
            WILL_INVOKE_CB(task_perform_cb_force_read, memdup(cb_data, sizeof(cb_data)));

            // expect a SIGN task then a WRITE task then when the zone is written
            // expect that the outbound serial will be the first of 0-99 serial
            // numbers for the current (mocked) date.ma
            EXPECT_TASK(TASK_FORCEREAD);
            EXPECT_TASK(TASK_SIGN);
            EXPECT_TASK(TASK_WRITE);
            outbound_serial_str = YMDN_TO_DATECOUNTER(y, m, d, n);
            EXPECT_SERIAL(outbound_serial_str);
        }
    }

    // Run the worker - arrange for it to stop after the tasks define above.
    e2e_go(state, TASK_STOP);
}

E2E_TEST_BEGIN(datecounter_serial)
    datecounter_serial_common(state, 24);
E2E_TEST_END

E2E_TEST_BEGIN(datecounter_serial_intra_day)
    datecounter_serial_common(state, 12);
E2E_TEST_END



// E2E_TEST_BEGIN(load_zone_file)
//     const char *zone_soa_serial = "2019111801";
//     uint32_t zone_soa_serial_num = atol(zone_soa_serial);
//     const char *replacements[] = { MOCK_ZONE_NAME, zone_soa_serial };
//     const char *test_zone = from_template(ZONE_TEMPLATE, replacements);

//     // Given an unsigned input zone
//     zone_type *zone = e2e_configure_mocks(state, test_zone, E2E_VIEWSTATE_W_OKAY);

//     // When it is signed using the 'datecounter' soa serial algorithm
//     zone->signconf->soa_serial = strdup("datecounter");
//     zone->signconf->last_modified = 1;
//     will_return_always(__wrap_time_now, __real_time_now());

//     // Expect the serial number to be
//     expect_output_zone_serials(&((zone_type) {
//         .name           = zone->name,
//         .nextserial     = NULL,
//         .inboundserial  = SERIALDUPA(zone_soa_serial_num),
//         .outboundserial = SERIALDUPA(zone_soa_serial_num+1)
//     }));

//     // Go!
//     e2e_go(state, TASK_READ, TASK_SIGN, TASK_WRITE, TASK_STOP);
// E2E_TEST_END


// E2E_TEST_BEGIN(load_zone_file2)
//     const char *zone_soa_serial = "2019111801";
//     uint32_t zone_soa_serial_num = atol(zone_soa_serial);
//     const char *replacements[] = { MOCK_ZONE_NAME, zone_soa_serial };
//     const char *test_zone = from_template(ZONE_TEMPLATE, replacements);

//     // sleeping >0.5 seconds and then loading state which contains an expiry
//     // time for a record in the zone to be signed (in this case bill.MOCKZONE.)
//     // causes an assert:
//     // src/views/recordset.c:623: names_recordsetexpiry: Assertion `record->expiry == NULL' failed.
//     usleep(600000);

//     // Given an unsigned input zone and prior state
//     zone_type *zone = e2e_configure_mocks(state, test_zone, E2E_VIEWSTATE_R_OKAY);

//     // When it is signed using the 'datecounter' soa serial algorithm
//     zone->signconf->soa_serial = strdup("datecounter");
//     zone->signconf->last_modified = 1;
//     will_return_always(__wrap_time_now, __real_time_now());

//     // Expect the serial number to be
//     expect_output_zone_serials(&((zone_type) {
//         .name           = zone->name,
//         .nextserial     = NULL,
//         .inboundserial  = SERIALDUPA(zone_soa_serial_num),
//         .outboundserial = SERIALDUPA(zone_soa_serial_num+1)
//     }));

//     // Go!
//     e2e_go(state, TASK_READ, TASK_SIGN, TASK_WRITE, TASK_STOP);
// E2E_TEST_END


// E2E_TEST_BEGIN(cleanup_view_state_tmp_file)
//     unlink(MOCK_VIEW_STATE_PATH);
// E2E_TEST_END