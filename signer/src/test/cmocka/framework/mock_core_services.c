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
#include <fcntl.h>


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
        TEST_LOG("mock") "Returning mock FILE* from ods_fopen() '%s' in dir '%s' with mode '%s'\n", file, dir, mode);
        return MOCK_POINTER;
    } else {
        TEST_LOG("mock") "Deliberately failing to ods_fopen() '%s' in dir '%s' with mode '%s'\n", file, dir, mode);
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

// Wrap direct I/O calls to enable interception of views.c and marshalling.c
// direct .state(.tmp) file access.
int __wrap_open(const char *__path, int __oflag, ...)
{
    MOCK_ANNOUNCE()

    const char *file_ext = strchr(__path, '.');
    const char *mock_target = NULL;
    char *mock_action = NULL;
    e2e_view_state_mode mode = mock();

    if ((__oflag & O_RDWR) && 0 == strcmp(file_ext, ".state")) {
        mock_action = "READ";
        if (mode & E2E_VIEWSTATE_R_OKAY) {
            // Allow ODS to read view state
            mock_target = MOCK_VIEW_STATE_PATH;
        }
    }
    else if ((__oflag & O_CREAT) && 0 == strcmp(file_ext, ".state.tmp")) {
        mock_action = "WRITE";
        if (mode & E2E_VIEWSTATE_W_OKAY) {
            // Allow ODS to write view state
            mock_target = MOCK_VIEW_STATE_PATH;
        } else {
            // Prevent ODS from writing view state, but pretend to allow it
            // otherwise it gets upset
            mock_target = MOCK_FD;
        }
    }

    if (MOCK_FD == mock_target) {
        TEST_LOG("mock") "Preventing open(%s) view state by pretending to open() '%s'\n", mock_action, __path);
        return MOCK_FD;
    } else if (mock_target) {
        TEST_LOG("mock") "Redirecting open(%s) '%s' to open '%s' instead\n", mock_action, __path, mock_target);
        // We don't know how many arguments were passed but in this specific
        // case just for restoring a state file there are no additional
        // arguments. The calling code at the time of writing sets mode 0666
        // and we fail to read the file in a later test if we don't do the
        // same.
        return __real_open(mock_target, __oflag, 0666);
    } else {
        TEST_LOG("mock") "Deliberately failing open() '%s'\n", __path);
        return -1;
    }
}
int __wrap_openat(int __fd, const char *__path, int __oflag, ...)
{
    MOCK_ANNOUNCE()
    TEST_LOG("mock") "Deliberately failing to openat() '%s'\n", __path);
    return -1;
}
ssize_t __wrap_read(int __fd, void *__buf, size_t __nbytes)
{
    MOCK_ANNOUNCE()
    return __real_read(__fd, __buf, __nbytes);
}
ssize_t __wrap_write(int __fd, const void *__buf, size_t __n)
{
    // don't announce -- too verbose
    if (__fd == MOCK_FD) {
        // pretend to have successfully written the given number of bytes
        return __n;
    } else {
        return __real_write(__fd, __buf, __n);
    }
}
int __wrap_close(int __fd)
{
    MOCK_ANNOUNCE()
    return __fd == MOCK_FD ? 0 : __real_close(__fd);
}
int __wrap_renameat(int __oldfd, const char *__old, int __newfd,
		     const char *__new)
{
    // pretend to have done the rename
    return 0;
}