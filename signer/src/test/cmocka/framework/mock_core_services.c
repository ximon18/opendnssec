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
#include <stdlib.h>


#include "test_framework.h"


void set_mock_time_now_value(time_t t)
{
    will_return(__wrap_time_now, t);
}


// ----------------------------------------------------------------------------
// monkey patches:
// these require compilation with -Wl,--wrap=ods_log_debug,--wrap=xxx etc to 
// cause these wrapper implementations to replace the originals.
// ----------------------------------------------------------------------------

int __wrap_pthread_cond_init (pthread_cond_t *__restrict __cond,
			      const pthread_condattr_t *__restrict __cond_attr)
{
    return 0;
}
int __wrap_pthread_mutex_init (pthread_mutex_t *__mutex,
			       const pthread_mutexattr_t *__mutexattr)
{
    return 0;
}
int __wrap_pthread_mutex_lock (pthread_mutex_t *__mutex)
{
    return 0;
}
int __wrap_pthread_mutex_unlock (pthread_mutex_t *__mutex)
{
    return 0;
}
time_t __wrap_time_now(void)
{
    MOCK_ANNOUNCE();
    return mock();
}
FILE* __wrap_ods_fopen(const char* file, const char* dir, const char* mode)
{
    MOCK_ANNOUNCE();
    if (0 == strcmp(file, MOCK_ZONE_FILE_NAME)) {
        TEST_LOG("deeebug") "Returning mock FILE* on attempt to open file %s in dir %s with mode %s\n", file, dir, mode);
        return MOCK_POINTER;
    } else {
        TEST_LOG("error") "Returning NULL FILE* on attempt to open file %s in dir %s with mode %s\n", file, dir, mode);
        return 0;
    }
}
void __wrap_ods_fclose(FILE* fd)
{
    MOCK_ANNOUNCE();
    // nothing to do
}
int __wrap_ods_fgetc(FILE* fd, unsigned int* line_nr)
{
    if (fd == MOCK_POINTER) {
        int c = mock();
        if ((char)c == '\n') (*line_nr)++;
        return c;
    } else {
        fail();
    }
}
ods_status __wrap_privdrop(const char *username, const char *groupname, const char *newroot, uid_t* puid, gid_t* pgid)
{
    MOCK_ANNOUNCE();
    return ODS_STATUS_OK;
}
void __wrap_ods_chown(const char* file, uid_t uid, gid_t gid, int getdir)
{
    MOCK_ANNOUNCE();
}
int __wrap_util_write_pidfile(const char* pidfile, pid_t pid)
{
    MOCK_ANNOUNCE();
    return 0;
}
ods_status __wrap_parse_file_check(const char* cfgfile, const char* rngfile)
{
    MOCK_ANNOUNCE()
    return ODS_STATUS_OK;
}