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
#include <stdbool.h>


#include "test_fw.h"
#include "poc_e2e_test.h"


#define is_cmdlineflag_set(long_needle, short_needle) \
    (get_cmdlinearg(argc, argv, long_needle, false, NULL) ? true : false) || \
    (get_cmdlinearg(argc, argv, short_needle, false, NULL) ? true : false)

#define get_cmdlinearg_value(needle) \
    get_cmdlinearg(argc, argv, needle, true, NULL)

#define get_cmdlinearg_value_or_default(needle, def_value) \
    get_cmdlinearg(argc, argv, needle, true, def_value)


static char* get_cmdlinearg(
    int haystack_size, char** haystack, char *needle, bool want_value, char* def_value)
{
    for (int i = 0; i < haystack_size; i++) {
        if (want_value) {
            char* equals_ptr = strchr(haystack[i], '=');
            if (equals_ptr) {
                int equals_idx = equals_ptr - haystack[i];
                int r = strncmp(haystack[i], needle, equals_idx);
                if (0 == r) {
                    return equals_ptr+1;
                }
            }
        } else if (0 == strcmp(haystack[i], needle)) {
            return needle;
        }
    }
    return def_value;
}


static struct CMUnitTest tests[] = {
    cmocka_unit_test_setup_teardown(e2e_test_load_zone_file, e2e_setup, e2e_teardown),
};
static const num_tests = (sizeof(tests)/sizeof(struct CMUnitTest));

int main(int argc, char *argv[])
{
    if (is_cmdlineflag_set("--help", "-h")) {
        fprintf(stderr, "Usage: %s [--help|--list]\n", argv[0]);
        fprintf(stderr, "       %s [--log=info|error|etc] [--test=(-)some*n?me]\n", argv[0]);
        return 0;
    }

    set_logging_level(get_cmdlinearg_value("--log"));
    set_filtered_tests(get_cmdlinearg_value_or_default("--test", "*"));

    if (is_cmdlineflag_set("--list", "-l")) {
        fprintf(stderr, "Listing all test names:\n");
        for (int i = 0; i < num_tests; i++) {
            fprintf(stderr, "  - %s\n", tests[i].name);
        }
        return 0;
    }

    return cmocka_run_group_tests(tests, NULL, NULL);
}