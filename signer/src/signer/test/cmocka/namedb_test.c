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

/*
 * SOA serial management testing.
 * 
 * NOTE: Any comparison of serials using < > or max() or arithmetic using '+'
 * would seem to ignore the possibility of wrap around which does not seem
 * right... See '!!!' annotations below.
 * 
 * namedb_struct:
 * ==============
 * OpenDNSSEC manages SOA serials using the namedb_struct, a so-called "domain
 * name database", which includes information about the zone, domains, denials
 * and the following SOA serial related fields (all of which are initialized by
 * namedb_create() and by signer cli cmd clear to zero):
 * 
 *   inbserial       - "inbound serial": set:
 *                     from backup field "inbound" when starting up.
 *                     from SOA RR RDATA SERIAL from incoming file or XFR.
 *   intserial       - "internal serial": set:
 *                     from backup field "internal" when starting up.
 *                     by namedb_update_serial() - see logic below. 
 *   outserial       - "outbound": set:
 *                     from backup field "outbound" when starting up.
 *                     from tools_output from signer/worker TASK_WRITE.
 *   altserial       - set to XXX provided via signer sign --serial XXX.
 *   serial_updated  - set to 1 by adapi_process_soa from adapi_process_rr
 *                     from adapi_add/del_rr from worker_perform_task in
 *                     signer/worker.c for TASK_READ, after calling
 *                     named_update_serial() and actually modifying the soa rr.
 *   force_serial    - set to 1 if signer sign --serial XXX was provided on the
 *                     cmd line and the serial was util_serial_gt() than
 *                     max(outserial, inb_ound_serial).        !!!
 *                     cleared by namedb_update_serial().
 *   have_serial     - used to indicate that this is not the first time this
 *                     zone has been seen and thus we have prior state.
 * 
 *                     set to 1 if signer sign clear fails.
 *                     set to 1 by tools_output() which does 'write zone to
 *                     output adapter'
 *                     set to 1 when loading backup data on startup.
 * 
 *                     If NOT set, namedb_update_serial() outputs log warnings
 *                     if the specified/calculated serial does not increase the
 *                     previous SOA serial. Does this field do anything else?
 *                     Why log warnings if this field is set?
 * 
 *                     Yes, in 'counter' mode if have_serial is set then an
 *                     arg_inbound_serial that does NOT increase the previous
 *                     SOA serial will be used anyway. When have_serial is not
 *                     set then in this case the fallback SOA SERIAL value
 *                     'prev' + 1 is used instead.
 * 
 *                     Also, if have_serial is set then in all modes except
 *                     'keep' the check against the previous SOA serial is done
 *                     either against outserial or arg_inbound_serial, but if
 *                     have_serial is NOT set the comparison is only against
 *                     arg_inbound_serial. So setting have_serial _may_ cause
 *                     a proposed serial number to be based on the 'outserial'
 *                     rather than the arg_inbound_serial...
 * 
 *                     And also: if have_serial is NOT set then intserial is
 *                     set by namedb_update_serial() to the newly chosen value,
 *                     but when set the newly chosen value is not used directly
 *                     but rather only after "updating" it. See below.
 * 
 *                     xfrd_recover() only reads .state files in have_serial is
 *                     set.
 * 
 * nameb_update_serial() arg "inbound_serial" client logic:
 * ========================================================
 * NOTE: arg_inbound_serial is used by namedb_update_serial() instead of
 * inbserial in the namedb_struct, though it does log inbserial... how does
 * the inbound_serial argument relate to the inbserial field in the structure?
 * Answer: It is SOA RR RDATA SERIAL during TASK_READ, and inbserial during
 * TASK_SIGN.
 * 
 * 1. adapi_process_soa(zone, rr) sets arg_inbound_serial to the SOA RR RDATA
 *    "SERIAL" field defined in RFC 1035 section 3.3.13 SOA RDATA format as:
 *      "The unsigned 32 bit version number of the original copy of the zone.
 *       Zone transfers preserve this value.  This value wraps and should be
 *       compared using sequence space arithmetic."
 *    LDNS is used to extract the uint32 value from the SOA RR RDATA SERIAL field:
 *    ldns_rdf2native_int32(ldns_rr_rdf(rr, SE_SOA_RDATA_SERIAL))
 * 
 *    So the SOA SERIAL from the incoming data (be it file or XFR) is used as
 *    the arg_inbound_serial (ultimately called from addns.c/adfile.c via the
 *    adapi_add_rr() and adapi_del_rr() functions), which looks like it is in
 *    turn invoked ultimately in worker_perform_task in signer/worker.c for
 *    TASK_READ.
 *
 * 2. zone_update_serial() sets arg_inbound_serial to the inbserial field
 *    value (why!?! given that it already passes in the db object which has
 *    the inbserial value inside it???). This is invoked by worker_perform_task
 *    in signer/worker.c for TASK_SIGN.
 * 
 * According to code in signer/worker.c worker_perform_task() it seems that the
 * order of tasks is: TASK_SIGNCONF -> TASK_READ -> TASK_SIGN -> TASK_WRITE -v
 *                                                      ^---------------------
 * With both TASK_READ and TASK_SIGN causing calls to namedb_update_serial().
 * 
 * named_update_serial() logic:
 * ============================
 * NOTE: Whether the altserial set by signer sign --serial is used depends on
 * the status later when namedb_update_serial() processes the namedb_struct:
 * 
 * Key:
 *   < >  denote integer less than and greater than
 *
 *   GT   denotes util_serial_gt() which in turn uses the
 *        DNS_SERIAL_GT macro defined as:
 *          ((int)(((a) - (b)) & 0xFFFFFFFF) > 0)
 *        The comment for this macro is interesting:
 *          "copycode: This define is taken from BIND9"
 *        !!! This isn't at all compliant with RFC-1982 !!!
 * 
 * Logic:
 *   IF have_serial AND outserial > arg_inbound_serial:         !!!
 *     prev = outserial
 *   ELSE:
 *     prev = arg_inbound_serial
 *
 *   IF force_serial:
 *     IF alt_serial GT 'prev':
 *       soa = altserial
 *     ELSE:
 *       soa = 'prev' + 1                                         !!!
 *   ELIF format == 'unixtime':
 *     computed_soa = time_now()                                  [1]
 *     IF computed_soa GT 'prev':
 *       soa = computed_soa
 *     ELSE:
 *       soa = 'prev' + 1                                         !!!
 *       IF NOT have_serial: log warning
 *   ELIF format == 'datecounter':
 *     computed_soa = time_now(as %Y%m%d as uint32)               [1]
 *     IF computed_soa GT 'prev':
 *       soa = computed_soa
 *     ELSE:
 *       soa = 'prev' + 1                                         !!!
 *       IF NOT have_serial: log warning
 *   ELIF format == 'counter':
 *     computed_soa = arg_inbound_serial + 1                      !!!
 *     IF computed_soa GT 'prev':
 *       soa = computed_soa
 *     ELIF have_serial:                                          [2]
 *       soa = 'prev' + 1                                         !!!
 *   ELIF format == 'keep':
 *     computed_soa = arg_inbound_serial
 *     'prev' = outserial                                         [3]
 *     IF computed_soa GT outserial:                              [4]
 *       soa = computed_soa
 *   ELSE:
 *     error
 * 
 *   update = soa - 'prev'                                        [5]
 *   IF update > 0x7FFFFFFF:
 *     update = 0x7FFFFFFF
 *   IF have_serial:
 *     intserial = 'prev' + update
 *   ELSE:
 *     intserial = soa
 * 
 * Footnotes:
 *   [1] time_now() is either time(), or if BOTH the ENFORCER_TIMESHIFT define
 *       AND the ENFORCER_TIMESHIFT environment variable are set then
 *       time_now() is timeshift2time(env var value). Is this possible with the
 *       signer or only with the enforcer? Note that this feature is for
 *       debugging so not important for understanding the correct busines
 *       logic here.
 *   [2] Why only in 'counter' format is 'prev' + 1 not used if
 *       have_serial is not set?
 *   [3] In 'keep' mode 'prev' is forced to outserial, which only affects the
 *       final soa if have_serial is set and thus 'update' arithmetic is done
 *       (see [5]), i.e. undoing the forced use of inbound_serial as is usually
 *       used for 'prev' when have_serial is set.
 *   [4] In 'keep' mode outserial is compared to, in other modes outserial is
 *       compared to only if outserial > arg_inbound_serial AND have_serial.
 */

// cmocka required includes
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

// additional includes used by this test suite
#include <string.h>
#include <stdio.h>
#include <math.h>

// includes for the ODS code to be tested
#include "signer/namedb.h"
#include "daemon/worker.h"
#include "daemon/engine.h"
#include "shared/util.h"

// useful constants
const int SERIAL_BITS = 32;
const uint64_t TWO_POW_32 = 4294967296;         // 2^(SERIAL_BITS)
const uint32_t TWO_POW_31 = 2147483648;         // 2^(SERIAL_BITS - 1)
const uint32_t MAX_SERIAL_ADD = TWO_POW_31 - 1; // 2^(SERIAL_BITS - 1) - 1
const uint32_t MOCK_UNIXTIME_NOW = 1234;

// useful macros
#define MOCK_ASSERT(expression) \
    mock_assert((int)(expression), #expression, __FILE__, __LINE__);


// ----------------------------------------------------------------------------
// monkey patches:
// ----------------------------------------------------------------------------
// requires compilation with -Wl,--wrap=ods_log_debug,--wrap=xxx etc to cause
// these wrapper implementations to replace the originals.
#define WRAP_ODS_LOG(format) \
    va_list args; \
    va_start (args, format); \
    fprintf(stderr, "%s: ", __func__); \
    vfprintf(stderr, format, args); \
    fprintf(stderr, "\n"); \
    va_end(args);

void __wrap_ods_fatal_exit(const char *format, ...)  { WRAP_ODS_LOG(format); }
void __wrap_ods_log_debug(const char *format, ...)   { WRAP_ODS_LOG(format); }
void __wrap_ods_log_verbose(const char *format, ...) { WRAP_ODS_LOG(format); }
void __wrap_ods_log_warning(const char *format, ...)
{
    WRAP_ODS_LOG(format);
    check_expected(format);
}
void __wrap_ods_log_error(const char *format, ...)
{
    WRAP_ODS_LOG(format);
    check_expected(format);
}
time_t __wrap_time_now(void)
{
    return mock();
}


// ----------------------------------------------------------------------------
// cmocka check_expect() plugin functions:
// usage:
//   expect_check(__wrap_ods_log_warning, format, check_contains, partial_msg);
// ----------------------------------------------------------------------------
static int check_contains(const LargestIntegralType value, const LargestIntegralType check_value_data)
{
    const char* needle = (const char*) check_value_data;
    const char* haystack = (const char*) value;
    char* match = strstr(haystack, needle);
    if (match != NULL) {
        return 1;
    } else {
        fprintf(stderr, "check_contains: '%s' not found in '%s'\n", needle, haystack);
        return 0;
    }
}


// ----------------------------------------------------------------------------
// custom cmocka-like expect() functions:
// ----------------------------------------------------------------------------
static void expect_ods_log_error(const char* partial_msg)
{
    expect_check(__wrap_ods_log_error, format, check_contains, partial_msg);
}

static void expect_ods_log_warning(const char* partial_msg)
{
    expect_check(__wrap_ods_log_warning, format, check_contains, partial_msg);
}


// ----------------------------------------------------------------------------
// cmocka() mock helpers.
// ----------------------------------------------------------------------------
static void set_mock_time_now_value(time_t t)
{
    will_return(__wrap_time_now, t);
}


// ----------------------------------------------------------------------------
// helper macros and functions used by the tests.
// ----------------------------------------------------------------------------
#define namedb_update_serial_test_common() \
    {0}; \
    will_return(__wrap_time_now, 0);

static void rfc1982_serial_increment(uint32_t s, uint32_t n)
{
    // RFC-1982 section "3.1. Addition":
    // Serial numbers may be incremented by the addition of a positive
    // integer n, where n is taken from the range of integers
    // [0 .. (2^(SERIAL_BITS - 1) - 1)].  For a sequence number s, the
    // result of such an addition, s', is defined as
    // 
    //               s' = (s + n) modulo (2 ^ SERIAL_BITS)
    //
    // Addition of a value outside the range
    // [0 .. (2^(SERIAL_BITS - 1) - 1)] is undefined.
    assert_true(n <= MAX_SERIAL_ADD);
    return (uint32_t) (((uint64_t)s + n) % TWO_POW_32);
}

static int rfc1982_serial_comparison(uint32_t i1_32, uint32_t i2_32)
{
    // RFC-1982 section "3.2. Comparison":
    // For the purposes of this definition, consider two integers, i1 and
    // i2, from the unbounded set of non-negative integers, such that i1 and
    // s1 have the same numeric value, as do i2 and s2.  Arithmetic and
    // comparisons applied to i1 and i2 use ordinary unbounded integer
    // arithmetic.
    //
    // Then, s1 is said to be equal to s2 if and only if i1 is equal to i2,
    // in all other cases, s1 is not equal to s2.
    //
    // s1 is said to be less than s2 if, and only if, s1 is not equal to s2,
    // and
    //     (i1 < i2 and i2 - i1 < 2^(SERIAL_BITS - 1)) or
    //     (i1 > i2 and i1 - i2 > 2^(SERIAL_BITS - 1))
    //
    // s1 is said to be greater than s2 if, and only if, s1 is not equal to
    // s2, and
    //     (i1 < i2 and i2 - i1 > 2^(SERIAL_BITS - 1)) or
    //     (i1 > i2 and i1 - i2 < 2^(SERIAL_BITS - 1))
    int64_t i1 = i1_32;
    int64_t i2 = i2_32;

    if (i1 == i2) {
        // s1 equal s2
        return 0;
    } else if ((i1 < i2 && i2 - i1 < TWO_POW_31) ||
               (i1 > i2 && i1 - i2 > TWO_POW_31)) {
        // s1 less than s2 
        return -1;
    } else if ((i1 < i2 && i2 - i1 > TWO_POW_31) ||
               (i1 > i2 && i1 - i2 < TWO_POW_31)) {
        // s1 greater than s2
        return 1;
    } else {
        // RFC-1982 section "3.2. Comparison":
        // Note that there are some pairs of values s1 and s2 for which s1 is
        // not equal to s2, but for which s1 is neither greater than, nor less
        // than, s2.  An attempt to use these ordering operators on such pairs
        // of values produces an undefined result.
        // ...
        // Thus the problem case is left undefined, implementations are free to
        // return either result, or to flag an error, and users must take care
        // not to depend on any particular outcome.  Usually this will mean
        // avoiding allowing those particular pairs of numbers to co-exist.
        //
        // This will happen where (pseudo code): abs(i1 - i2) == 2^31
        // Raise a mock assertion that can be cmocka expected in tests below.
        MOCK_ASSERT(false);
    }
}

static int rfc1982_serial_gt(uint32_t i1, uint32_t i2)
{
    return rfc1982_serial_comparison(i1, i2) > 0;
}

static void test_one_serial_gt(uint32_t i1, uint32_t i2)
{
    assert_int_equal(util_serial_gt(i1, i2), rfc1982_serial_gt(i1, i2));
}


// ----------------------------------------------------------------------------
// the tests:
// TODO: test all the possible permutations of things like have_serial,
// arg_inbound_serial, inbserial, 'format' and outbserial.
// ----------------------------------------------------------------------------
static void test_soa_serial_counter(void **unused)
{
    namedb_type db = namedb_update_serial_test_common();

    // given a previously seen inbound serial number
    db.have_serial = 1;
    db.intserial = 0;

    // and an inbound serial number that is equal to the previous serial
    uint32_t inbound_serial = db.intserial;

    // when updating the serial number succeeds
    assert_int_equal(
        ODS_STATUS_OK,
        namedb_update_serial(&db, "azone", "counter", inbound_serial));

    // verify that the inbound serial number was incremented by one
    assert_int_equal(db.intserial, inbound_serial + 1);
}

static void test_soa_serial_keep_okay(void **unused)
{
    namedb_type db = namedb_update_serial_test_common();

    // given a previously seen inbound serial number
    db.have_serial = 1;
    db.intserial = 0;

    // and an inbound serial number that is higher than the previous serial
    uint32_t inbound_serial = db.intserial + 1;

    // when updating the serial number succeeds
    assert_int_equal(
        ODS_STATUS_OK,
        namedb_update_serial(&db, "azone", "keep", inbound_serial));

    // verify that the specified inbound serial number was "kept"
    assert_int_equal(db.intserial, inbound_serial);
}

static void test_soa_serial_keep_unusable(void **unused)
{
    namedb_type db = namedb_update_serial_test_common();

    // given a previously seen inbound serial number
    db.have_serial = 1;
    db.intserial = 0;

    // and an inbound serial number that is unchanged from the previous serial
    uint32_t inbound_serial = db.intserial;

    // expect an error to be logged
    expect_ods_log_error("cannot keep SOA SERIAL from input zone");

    // when failing to update the serial number
    assert_int_equal(
        ODS_STATUS_CONFLICT_ERR,
        namedb_update_serial(&db, "azone", "keep", inbound_serial));
}

static void test_soa_serial_unixtime_gt_now(void **unused)
{
    namedb_type db = namedb_update_serial_test_common();

    // given a previously seen inbound serial number
    db.have_serial = 1;
    db.intserial = 0;

    // given that the time now is controlled by us
    set_mock_time_now_value(MOCK_UNIXTIME_NOW);

    // and that the inbound serial number is newer than the time now
    uint32_t inbound_serial = MOCK_UNIXTIME_NOW + 1;

    // when updating the serial number succeeds
    assert_int_equal(
        ODS_STATUS_OK,
        namedb_update_serial(&db, "azone", "unixtime", inbound_serial)),

    // verify that the chosen serial number is greater than both the previous
    // serial and the inbound serial
    assert_int_equal(db.intserial, inbound_serial + 1);
}

static void test_soa_serial_unixtime_eq_now(void **unused)
{
    namedb_type db = namedb_update_serial_test_common();

    // given a previously seen inbound serial number
    db.have_serial = 1;
    db.intserial = 0;

    // given that the time now is controlled by us
    set_mock_time_now_value(MOCK_UNIXTIME_NOW);

    // and that the inbound serial number is the same as the time now
    uint32_t inbound_serial = MOCK_UNIXTIME_NOW;

    // when updating the serial number succeeds
    assert_int_equal(
        ODS_STATUS_OK,
        namedb_update_serial(&db, "azone", "unixtime", inbound_serial)),

    // verify that the newly chosen serial number has been forced to be one
    // higher than the unusable inbound serial number
    assert_int_equal(db.intserial, inbound_serial + 1);
}

static void test_soa_serial_unixtime_lt_now(void **unused)
{
    namedb_type db = namedb_update_serial_test_common();

    // given a previously seen inbound serial number
    db.have_serial = 1;
    db.intserial = 0;

    // given that the time now is controlled by us
    set_mock_time_now_value(MOCK_UNIXTIME_NOW);

    // and that the inbound serial number is older than the time now
    uint32_t inbound_serial = MOCK_UNIXTIME_NOW - 1;

    // when updating the serial number succeeds
    assert_int_equal(
        ODS_STATUS_OK,
        namedb_update_serial(&db, "azone", "unixtime", inbound_serial));

    // verify that the new serial is set to the time now
    assert_int_equal(db.intserial, MOCK_UNIXTIME_NOW);
}

static int worker_setup(void** state)
{
    namedb_type* db = calloc(1, sizeof(namedb_type));

    zone_type* zone = zone_create(strdup("blah."), LDNS_RR_CLASS_IN);
    ldns_rr* rrsoa = ldns_rr_new_frm_type(LDNS_RR_TYPE_SOA);
    ldns_rr_set_class(rrsoa, LDNS_RR_CLASS_IN);
    ldns_rr_set_ttl(rrsoa, 3600);
    ldns_rr_set_owner(rrsoa, ldns_rdf_clone(zone->apex));
    zone_add_rr(zone, rrsoa, 0);

    zone->signconf = calloc(1, sizeof(signconf_type));
    zone->signconf->soa_serial = strdup("unixtime");

    task_type* task = calloc(1, sizeof(task_type));
    task->zone = zone;

    engine_type* engine = calloc(1, sizeof(engine_type));

    worker_type* worker = calloc(1, sizeof(worker_type));
    worker->type = WORKER_WORKER;
    worker->task = task;
    worker->engine = engine;
    *state = worker;

    return 0;
}

static int worker_teardown(worker_type** state)
{
    worker_type* worker = *state;
    free(worker->engine);
    free(((zone_type*)worker->task->zone)->db);
    free(worker->task->zone);
    free(worker->task);
    free(worker);
    return 0;
}

// WORK IN PROGRESS
static void test_soa_serial_unixtime_lt_now_e2e(worker_type** state)
{
    // worker_start is the innermost function that is visible to us but calling
    // that involves setting up engine details that we're not interested in and
    // is a lot of extra boilerplate. Instead mark the worker_perform_task()
    // function as non-static when in test mode so that we can access it from
    // here. See: https://stackoverflow.com/a/49901081
    worker_type* worker = *state;
    worker->task->what = TASK_SIGN;
    will_return_always(__wrap_time_now, 1);
    expect_ods_log_error("failed to replace soa serial"); // see zone_update_serial()
    expect_ods_log_error("unable to sign zone");
    worker_perform_task(worker);
}

static void test_serial_gt(void **unused)
{
    // loosely based on RFC 1982 section "5.2. A slightly larger example".
    test_one_serial_gt(1, 0);
    test_one_serial_gt(44, 0);
    test_one_serial_gt(100, 0);
    test_one_serial_gt(100, 44);
    test_one_serial_gt(200, 100);
    test_one_serial_gt(TWO_POW_31, TWO_POW_31-55);
    test_one_serial_gt(100, TWO_POW_31);
    test_one_serial_gt(0, TWO_POW_31-55);
    test_one_serial_gt(44, TWO_POW_31-55);
}

static void test_serial_gt_incomparable(void **unused)
{
    // According to RFC-1982 sections "3.2. Comparison", section "5.1. A
    // trivial example" and "5.2. A slightly larger example" comparison of
    // some serial numbers, specifically those exactly half the serial range
    // apart, is undefined, and that " implementations are free to return
    // either result, or to flag an error". This test documents what ODS does
    // in such cases. Using rfc1982_serial_gt instead of util_serial_gt will
    // fail as it refuses to compare such numbers.
    uint32_t i1s[] = { TWO_POW_31, TWO_POW_31+1, TWO_POW_32-2, TWO_POW_32-1 };
    uint32_t i2s[] = { 0,          1,            TWO_POW_31-2, TWO_POW_31-1 };
    uint32_t idx;

    for (idx = 0; idx < sizeof(i1s)/sizeof(uint32_t); idx++) {
        uint32_t i1 = i1s[idx];
        uint32_t i2 = i2s[idx];
        assert_false(util_serial_gt(i1, i2));
        expect_assert_failure(rfc1982_serial_gt(i1, i2));
    }

    // note that the reverse comparisons give the same result
    for (idx = 0; idx < sizeof(i1s)/sizeof(uint32_t); idx++) {
        uint32_t i1 = i1s[idx];
        uint32_t i2 = i2s[idx];
        assert_false(util_serial_gt(i2, i1));
        expect_assert_failure(rfc1982_serial_gt(i2, i1));
    }
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_soa_serial_counter),
        cmocka_unit_test(test_soa_serial_keep_okay),
        cmocka_unit_test(test_soa_serial_keep_unusable),
        cmocka_unit_test(test_soa_serial_unixtime_gt_now),
        cmocka_unit_test(test_soa_serial_unixtime_eq_now),
        cmocka_unit_test(test_soa_serial_unixtime_lt_now),
        cmocka_unit_test(test_serial_gt),
        cmocka_unit_test(test_serial_gt_incomparable),
        cmocka_unit_test_setup_teardown(test_soa_serial_unixtime_lt_now_e2e, worker_setup, worker_teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}