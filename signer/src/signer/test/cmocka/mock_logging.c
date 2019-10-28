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
#include <stdarg.h>
#include <stdlib.h>
#include "mock_logging.h"
#include "test_framework.h"


#define DEFAULT_LOG_LEVEL      "warning"
#define LOG_LEVELS_LOW_TO_HIGH "mock,deeebug,debug,verbose,info,warning,error,crit,fatal"


static char *log_filter = "all";


static char* get_ge_log_levels(const char *log_level)
{
    char *log_level_slice = (
        0 == strcmp("all", log_level)
        ? LOG_LEVELS_LOW_TO_HIGH
        : strstr(LOG_LEVELS_LOW_TO_HIGH, log_level)
    );
    return log_level_slice ? log_level_slice : "";
}

bool log_level_enabled(const char *log_level)
{
    return 0 != strstr(log_filter, log_level);
}

void set_logging_level(const char *log_level)
{
    log_level = log_level ? log_level : getenv("TESTING_LOG_LEVEL");
    log_level = log_level ? log_level : DEFAULT_LOG_LEVEL;
    if (log_level) {
        log_filter = get_ge_log_levels(log_level);
    }
    TEST_LOG("info") "Enabled logging levels: %s\n", log_filter);
}

void set_filtered_tests(const char *filter)
{
    if (filter) {
        if (strchr(filter, '-') == filter) {
            char *actual_filter = filter + 1;
            TEST_LOG("info") "Skip tests matching: %s\n", actual_filter);
            cmocka_set_skip_filter(actual_filter);
        } else {
            TEST_LOG("info") "Run tests matching: %s\n", filter);
            cmocka_set_test_filter(filter);
        }
    }
}

#define WRAP_VARARGS_LOGN(level, format) \
    WRAP_VARARGS_LOG_WITH_EOL(level, format, "\n")

#define WRAP_VARARGS_LOG(level, format) \
    WRAP_VARARGS_LOG_WITH_EOL(level, format, "")

#define WRAP_VARARGS_LOG_WITH_EOL(level, format, eol) \
    if (log_level_enabled(level)) { \
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

// ----------------------------------------------------------------------------
// monkey patches:
// these require compilation with -Wl,--wrap=ods_log_debug,--wrap=xxx etc to 
// cause these wrapper implementations to replace the originals.
// ----------------------------------------------------------------------------
void __wrap_print_message(const char* const format, ...) {
    // Silence irritating CMocka printf output for which no off switch exists.
    if (!strcmp(format, "Expected assertion false occurred")) {
        WRAP_VARARGS_LOG("mock", format);
    }
}

int  __wrap_ods_log_get_level()                      { return 999; } // pass everything thru to us
void __wrap_ods_log_deeebug(const char *format, ...) { WRAP_VARARGS_LOGN("deeebug", format); }
void __wrap_ods_log_debug(const char *format, ...)   { WRAP_VARARGS_LOGN("debug", format); }
void __wrap_ods_log_verbose(const char *format, ...) { WRAP_VARARGS_LOGN("verbose", format); }
void __wrap_ods_log_info(const char *format, ...)    { WRAP_VARARGS_LOGN("info", format); }
void __wrap_ods_log_warning(const char *format, ...) { WRAP_UNEXPECTED_ODS_LOG("warning", format); }
void __wrap_ods_log_error(const char *format, ...)   { WRAP_UNEXPECTED_ODS_LOG("error", format); }
void __wrap_ods_log_crit(const char *format, ...)    { WRAP_UNEXPECTED_ODS_LOG("crit", format); }
void __wrap_ods_fatal_exit(const char *format, ...)  { WRAP_UNEXPECTED_ODS_LOG("fatal", format); }

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
static void expect_ods_log_warning(const char* partial_msg) { expect_check(__wrap_ods_log_warning, format, check_contains, partial_msg); }
static void expect_ods_log_error(const char* partial_msg)   { expect_check(__wrap_ods_log_error, format, check_contains, partial_msg);   }
static void expect_ods_log_crit(const char* partial_msg)    { expect_check(__wrap_ods_log_crit, format, check_contains, partial_msg);    }
static void expect_ods_fatal_exit(const char* partial_msg)  { expect_check(__wrap_ods_fatal_exit, format, check_contains, partial_msg);  }