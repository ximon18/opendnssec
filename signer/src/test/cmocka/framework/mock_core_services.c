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
#include <time.h>
#include <stdio.h>


#include "test_framework.h"


// ----------------------------------------------------------------------------
// monkey patches:
// these require compilation with -Wl,--wrap=ods_log_debug,--wrap=xxx etc to 
// cause these wrapper implementations to replace the originals.
// ----------------------------------------------------------------------------


// Mock locks for thread synchronisation as we run the tests in a single thread
int __wrap_pthread_mutex_lock (pthread_mutex_t *__mutex) { return 0; }
int __wrap_pthread_mutex_unlock (pthread_mutex_t *__mutex) { return 0; }


// Mock time
time_t __wrap_time_now(void)
{
    MOCK_ANNOUNCE();
    return mock();
}

void set_mock_time_now_value(time_t t)
{
    will_return(__wrap_time_now, t);
}


// Mock file access
FILE* __wrap_ods_fopen(const char* file, const char* dir, const char* mode)
{
    MOCK_ANNOUNCE();
    return MOCK_POINTER; // override fgetc and return data based on the ptr returned here?
}
void __wrap_ods_fclose(FILE* fd) {
    MOCK_ANNOUNCE();
    // nothing to do
}
int __wrap_ods_fgetc(FILE* fd, unsigned int* line_nr)
{
    // MOCK_ANNOUNCE();
    if (fd == MOCK_POINTER) {
        int c = mock();
        if ((char)c == '\n') (*line_nr)++;
        return c;
    } else {
        fail();
    }
}