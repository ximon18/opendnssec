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
 * SOA serial management testing using CMocka.
 * 
 * To run: make check
 * Or: TESTING_LOG_LEVEL="all" make check
 * Test log output is written to signer/src/test-suite.log by default.
 *
 * Assumes that src/signer/Makefile.am is configured to build a separate
 * check binary using linker arguments -Wl,--wrap=ods_log_debug,--wrap=... for
 * each of the __wrap_ functions defined below.
 */


// TODO: split me out into separate files.


// cmocka required includes
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

// additional includes used by this test suite
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <pkcs11.h>

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
#define MOCK_HSM_MODULE_NAME "MOCK_MODULE"


// useful macros
#define MOCK_ASSERT(expression) \
    mock_assert((int)(expression), #expression, __FILE__, __LINE__);
#define MOCK_POINTER (void*)0xDEADBEEF


// ----------------------------------------------------------------------------
// monkey patches:
// these require compilation with -Wl,--wrap=ods_log_debug,--wrap=xxx etc to 
// cause these wrapper implementations to replace the originals.
// ----------------------------------------------------------------------------

// Mock logging
char *log_filter = "warning|error|crit";
#define LOG_LEVEL_ENABLED(level) \
    0 == strcmp(log_filter, "all") || strstr(log_filter, level)

#define TEST_LOG_NOCHECK(level) \
    fprintf(stderr, "test: log[" level "] "

#define TEST_LOG(level) \
    if (LOG_LEVEL_ENABLED(level)) TEST_LOG_NOCHECK(level)

#define WRAP_VARARGS_LOGN(level, format) \
    WRAP_VARARGS_LOG_WITH_EOL(level, format, "\n")

#define WRAP_VARARGS_LOG(level, format) \
    WRAP_VARARGS_LOG_WITH_EOL(level, format, "")

#define WRAP_VARARGS_LOG_WITH_EOL(level, format, eol) \
    if (LOG_LEVEL_ENABLED(level)) { \
        va_list args; \
        va_start (args, format); \
        TEST_LOG_NOCHECK(level)); \
        vfprintf(stderr, format, args); \
        fprintf(stderr, eol); \
        va_end(args); \
    }

#define WRAP_UNEXPECTED_ODS_LOG(level, format) \
    WRAP_VARARGS_LOGN(level, format); \
    check_expected(format);

void __wrap_print_message(const char* const format, ...) {
    // Silence irritating CMocka printf output for which no off switch exists.
    if (!strcmp(format, "Expected assertion false occurred")) {
        WRAP_VARARGS_LOG("mock", format);
    }
}
void __wrap_ods_fatal_exit(const char *format, ...)  { WRAP_VARARGS_LOGN("fatal", format); }
void __wrap_ods_log_debug(const char *format, ...)   { WRAP_VARARGS_LOGN("debug", format); }
void __wrap_ods_log_deeebug(const char *format, ...) { WRAP_VARARGS_LOGN("deeebug", format); }
void __wrap_ods_log_verbose(const char *format, ...) { WRAP_VARARGS_LOGN("verbose", format); }
void __wrap_ods_log_info(const char *format, ...)    { WRAP_VARARGS_LOGN("info", format); }
void __wrap_ods_log_warning(const char *format, ...) { WRAP_UNEXPECTED_ODS_LOG("warning", format); }
void __wrap_ods_log_error(const char *format, ...)   { WRAP_UNEXPECTED_ODS_LOG("error", format); }
void __wrap_ods_log_crit(const char *format, ...)    { WRAP_UNEXPECTED_ODS_LOG("crit", format); }

#define MOCK_ANNOUNCE() \
    TEST_LOG("mock") "mock: %s()\n", __func__);


// Mock time
time_t __wrap_time_now(void)
{
    MOCK_ANNOUNCE();
    return mock();
}

// Mock HSM
CK_RV mock_C_DigestInit(unsigned long session, CK_MECHANISM_PTR m) {
    MOCK_ANNOUNCE();
    return CKR_OK;
}
CK_RV mock_C_Digest(unsigned long session, unsigned char* data,
    unsigned long data_len, unsigned char* digest, unsigned long *digest_len) {
    static unsigned char MOCK_DIGEST[] = { 0xD, 0xE, 0xA, 0xD, 0xB, 0xE, 0xE, 0xF };
    MOCK_ANNOUNCE();
    memcpy(digest, MOCK_DIGEST, sizeof(MOCK_DIGEST));
    *digest_len = sizeof(MOCK_DIGEST);
    return CKR_OK;
}
CK_RV mock_C_SignInit(unsigned long session, CK_MECHANISM_PTR m, unsigned long pk) {
    MOCK_ANNOUNCE();
    return CKR_OK;
}
CK_RV mock_C_Sign(unsigned long session, unsigned char* data,
    unsigned long data_len, unsigned char* signature, unsigned long *signature_len) {
    static unsigned char MOCK_SIGNATURE[] = { 0xD, 0xE, 0xA, 0xD, 0xB, 0xE, 0xE, 0xF };
    MOCK_ANNOUNCE();
    function_called();
    memcpy(signature, MOCK_SIGNATURE, sizeof(MOCK_SIGNATURE));
    *signature_len = sizeof(MOCK_SIGNATURE);
    return CKR_OK;
}
hsm_ctx_t * __wrap_hsm_create_context() {
    MOCK_ANNOUNCE();
    return mock();
}
int __wrap_hsm_check_context() {
    MOCK_ANNOUNCE();
    return HSM_OK;
}
void __wrap_hsm_destroy_context(hsm_ctx_t *ctx) {
    MOCK_ANNOUNCE();
    // Destruction is handled by the test teardown function.
}
ldns_rr * __wrap_hsm_get_dnskey(
    hsm_ctx_t *ctx, const hsm_key_t *key, const hsm_sign_params_t *sign_params) {
    MOCK_ANNOUNCE();
    return mock();
}
/*ldns_rr* __wrap_hsm_sign_rrset(
    hsm_ctx_t *ctx, const ldns_rr_list* rrset, const hsm_key_t *key,
    const hsm_sign_params_t *sign_params) {
    MOCK_ANNOUNCE();
    // assert_ptr_equal(MOCK_POINTER, key);
    ldns_rr_list_print(stderr, rrset);
    ldns_rr *signature = hsm_create_empty_rrsig(rrset, sign_params);
    fprintf(stderr, "XIMON: hsr: sig=%p\n", signature);
    ldns_rr_print(stderr, signature);

    unsigned char sig_bytes[10] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 };
    ldns_rdf *b64_rdf = ldns_rdf_new_frm_data(LDNS_RDF_TYPE_B64, 10, sig_bytes);
    fprintf(stderr, "XIMON: hsr: b64=%p\n", b64_rdf);
    ldns_rr_rrsig_set_sig(signature, b64_rdf);
    fprintf(stderr, "XIMON: hsr: sig=%p\n", signature);
    // return MOCK_POINTER;
    return signature;
}*/
char * __wrap_hsm_get_error(hsm_ctx_t *gctx) { return NULL; }


// Mock locks for thread synchronisation as we run the tests in a single thread
int __wrap_pthread_mutex_lock (pthread_mutex_t *__mutex) { return 0; }
int __wrap_pthread_mutex_unlock (pthread_mutex_t *__mutex) { return 0; }


// Disable outputs during testing
ods_status __wrap_adapter_write(void* zone) {
    MOCK_ANNOUNCE();
    function_called();
    return ODS_STATUS_OK;
}
ods_status __wrap_zone_backup2(zone_type* zone) {
    MOCK_ANNOUNCE();
    return ODS_STATUS_OK;
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

const hsm_key_t* keylookup(hsm_ctx_t* ctx, const char* locator) {
    MOCK_ANNOUNCE();
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
        TEST_LOG("crit") "check_contains: '%s' not found in '%s'\n", needle, haystack);
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

static uint32_t rfc1982_serial_increment(uint32_t s, uint32_t n)
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

// e2e style testing support

typedef struct e2e_test_state_struct {
    worker_type *worker;
    hsm_ctx_t *hsm_ctx;
    hsm_key_t *hsm_key;
} e2e_test_state_type;

static int e2e_setup(void** state)
{
    // build a minimal zone
    zone_type* zone = zone_create(strdup("blah."), LDNS_RR_CLASS_IN);
    zone->signconf = signconf_create();
    zone->signconf->sig_resign_interval = duration_create_from_string("P1D");
    zone->signconf->keys = keylist_create(zone->signconf);
    zone->adoutbound = adapter_create("MOCK_CONFIG_STR", -1, 0);

    // add a soa record to the zone
    ldns_rr* rrsoa = ldns_rr_new_frm_type(LDNS_RR_TYPE_SOA);
    ldns_rr_set_class(rrsoa, LDNS_RR_CLASS_IN);
    ldns_rr_set_ttl(rrsoa, 3600);
    ldns_rr_set_owner(rrsoa, ldns_rdf_clone(zone->apex));
    ldns_rdf* mname, *rname;
    ldns_str2rdf_str(&mname, "orgnameserver");
    ldns_str2rdf_str(&rname, "someone.orgnameserver.com.");
    ldns_rr_set_rdf(rrsoa, mname, 0); // MNAME
    ldns_rr_set_rdf(rrsoa, rname, 1); // RNAME
    ldns_rr_set_rdf(rrsoa, ldns_native2rdf_int32(LDNS_RDF_TYPE_INT32, 0), 2); // SERIAL
    ldns_rr_set_rdf(rrsoa, ldns_native2rdf_int32(LDNS_RDF_TYPE_INT32, 3600), 3); // REFRESH
    ldns_rr_set_rdf(rrsoa, ldns_native2rdf_int32(LDNS_RDF_TYPE_INT32, 1800), 4); // RETRY
    ldns_rr_set_rdf(rrsoa, ldns_native2rdf_int32(LDNS_RDF_TYPE_INT32, 7200), 5); // EXPIRE
    ldns_rr_set_rdf(rrsoa, ldns_native2rdf_int32(LDNS_RDF_TYPE_INT32, 60), 6); // MINIMUM
    if (ODS_STATUS_OK != zone_add_rr(zone, rrsoa, 0)) fail();

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

    // define a mock pkcs#11 function symbol table for use with the HSM context
    // built below.
    CK_FUNCTION_LIST_PTR mock_sym_table = calloc(1, sizeof(CK_FUNCTION_LIST));
    mock_sym_table->C_DigestInit = mock_C_DigestInit;
    mock_sym_table->C_Digest = mock_C_Digest;
    mock_sym_table->C_SignInit = mock_C_SignInit;
    mock_sym_table->C_Sign = mock_C_Sign;

    // prepare a mock HSM context so that we can use libhsm functionality
    // without a real/soft HSM.
    hsm_ctx_t *ctx = calloc(1, sizeof(hsm_ctx_t));
    ctx->session_count = 1;
    ctx->session[0] = calloc(1, sizeof(hsm_session_t));
    ctx->session[0]->session = 0;
    ctx->session[0]->module = calloc(1, sizeof(hsm_module_t));
    ctx->session[0]->module->name = strdup(MOCK_HSM_MODULE_NAME);
    ctx->session[0]->module->sym = mock_sym_table;

    // define a HSM key whose HSM module name matches the mock HSM context
    // we just created.
    hsm_key_t *hsm_key = calloc(1, sizeof(hsm_key_t));
    hsm_key->modulename = strdup(MOCK_HSM_MODULE_NAME);

    // expose the objects above to CMocka tests via their state argument
    e2e_test_state_type *e2e_test_state = calloc(1, sizeof(e2e_test_state_type));
    e2e_test_state->worker = worker;
    e2e_test_state->hsm_ctx = ctx;
    e2e_test_state->hsm_key = hsm_key;

    *state = e2e_test_state;
    return 0;
}

static int e2e_teardown(e2e_test_state_type** state)
{
    e2e_test_state_type* e2e_test_state = *state;
    worker_type* worker = e2e_test_state->worker;

    // commented out because it causes test failure with error:
    //   munmap_chunk(): invalid pointer
    // zone_cleanup((zone_type*)worker->task->zone);
    free(worker->engine);
    free(worker->task);
    free(worker);

    free(e2e_test_state->hsm_ctx->session[0]->module->sym);
    free(e2e_test_state->hsm_ctx->session[0]->module->name);
    free(e2e_test_state->hsm_ctx->session[0]->module);
    free(e2e_test_state->hsm_ctx->session[0]);
    free(e2e_test_state->hsm_ctx);

    free(e2e_test_state->hsm_key);
    free(e2e_test_state);
    return 0;
}

static zone_type* e2e_common(e2e_test_state_type** state)
{
    e2e_test_state_type *e2e_test_state = *state;
    worker_type *worker = e2e_test_state->worker;

    // create a key for signing
    ldns_key *key_pair = ldns_key_new_frm_algorithm(LDNS_RSASHA256, 2048);
    ldns_rr *key_rr = ldns_key2rr(key_pair);
    zone_type *zone = worker->task->zone;
    keylist_push(zone->signconf->keys, "MOCK_LOCATOR",
        key_pair->_alg, LDNS_KEY_ZONE_KEY, 1, 0, 1, 0);

    ldns_rr* rra = ldns_rr_new_frm_type(LDNS_RR_TYPE_A);
    ldns_rr_set_class(rra, LDNS_RR_CLASS_IN);
    ldns_rr_set_ttl(rra, 3600);
    ldns_rr_set_owner(rra, ldns_rdf_clone(zone->apex));
    ldns_rdf* mname, *rname;
    ldns_rr_a_set_address(rra, ldns_native2rdf_int32(LDNS_RDF_TYPE_A,
        (9<<24)+(9<<16)+(9<<8)+(9<<0)));
    if (ODS_STATUS_OK != zone_add_rr(zone, rra, 0)) fail();

    // worker_start is the innermost function that is visible to us but calling
    // that involves setting up engine details that we're not interested in and
    // is a lot of extra boilerplate. Instead mark the worker_perform_task()
    // function as non-static when in test mode so that we can access it from
    // here. See: https://stackoverflow.com/a/49901081
    worker->task->what = TASK_SIGN;
    will_return_always(__wrap_hsm_get_dnskey, key_rr);
    will_return_always(keylookup, e2e_test_state->hsm_key);
    will_return_always(__wrap_hsm_create_context, e2e_test_state->hsm_ctx);
    will_return_always(worker_queue_rrset, e2e_test_state->hsm_ctx);
    expect_function_call(mock_C_Sign);
    // expect_function_call(__wrap_adapter_write);

    // return the zone as it is often what the test wants to manipulate
    return zone;
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

static void e2e_test_soa_serial_unixtime_gt_now(e2e_test_state_type** state)
{
    // Given
    zone_type *zone = e2e_common(state);
    zone->signconf->soa_serial = strdup("unixtime");
    zone->db->inbserial = 2;
    will_return_always(__wrap_time_now, zone->db->inbserial - 1);

    // When signing expect:
    expect_ods_log_warning("unable to use unixtime as serial: %u does not increase %u.");

    // Go!
    worker_perform_task((*state)->worker);
}

static void e2e_test_soa_serial_force_serial(e2e_test_state_type** state)
{
    // Given
    zone_type *zone = e2e_common(state);
    zone->signconf->soa_serial = strdup("unixtime");
    zone->db->inbserial = 2;
    zone->db->altserial = 4;
    zone->db->force_serial = 1;
    will_return_always(__wrap_time_now, zone->db->inbserial + 1);

    // When signing expect: no errors or warnings

    // Go!
    worker_perform_task((*state)->worker);
}

static void e2e_test_soa_serial_unable_to_force_serial(e2e_test_state_type** state)
{
    // Given
    zone_type *zone = e2e_common(state);
    zone->signconf->soa_serial = strdup("unixtime");
    zone->db->inbserial = 2;
    zone->db->altserial = 1;
    zone->db->force_serial = 1;
    will_return_always(__wrap_time_now, zone->db->inbserial + 1);

    // When signing expect:
    expect_ods_log_warning("unable to enforce serial: %u does not  increase %u");
    // Note the unexpected space here ----------------------------^

    // Go!
    worker_perform_task((*state)->worker);
}

int main(void)
{
    const char* log_level_env_var = getenv("TESTING_LOG_LEVEL");
    if (log_level_env_var) {
        log_filter = log_level_env_var;
    }

    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_soa_serial_counter),
        cmocka_unit_test(test_soa_serial_keep_okay),
        cmocka_unit_test(test_soa_serial_keep_unusable),
        cmocka_unit_test(test_soa_serial_unixtime_gt_now),
        cmocka_unit_test(test_soa_serial_unixtime_eq_now),
        cmocka_unit_test(test_soa_serial_unixtime_lt_now),
        cmocka_unit_test(test_serial_gt),
        cmocka_unit_test(test_serial_gt_incomparable),
        cmocka_unit_test_setup_teardown(e2e_test_soa_serial_unixtime_gt_now, e2e_setup, e2e_teardown),
        cmocka_unit_test_setup_teardown(e2e_test_soa_serial_force_serial, e2e_setup, e2e_teardown),
        cmocka_unit_test_setup_teardown(e2e_test_soa_serial_unable_to_force_serial, e2e_setup, e2e_teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}