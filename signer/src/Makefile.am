.PHONY: analyze
MAINTAINERCLEANFILES = $(srcdir)/Makefile.in

LIBHSM = ${top_builddir}/libhsm/src/lib/libhsm.a
LIBCOMPAT = ${top_builddir}/common/libcompat.a

AM_CPPFLAGS = \
	-I$(top_srcdir)/common \
	-I$(top_builddir)/common \
    -I$(top_srcdir)/libhsm/src/lib \
	@SSL_INCLUDES@ \
	@XML2_INCLUDES@ \
	@LDNS_INCLUDES@

signerdir =     @libdir@/opendnssec/signer

sbin_PROGRAMS = ods-signerd ods-signer
# man8_MANS =     man/ods-signer.8 man/ods-signerd.8

SIGNER_NON_MAIN_SOURCES = adapter/adapi.c adapter/adapi.h \
				adapter/adapter.c adapter/adapter.h \
				adapter/addns.c adapter/addns.h \
				adapter/adfile.c adapter/adfile.h \
				adapter/adutil.c adapter/adutil.h \
				daemon/metastorage.c daemon/metastorage.h \
				daemon/signercommands.c daemon/signercommands.h \
				daemon/signeroperation.c \
				daemon/dnshandler.c daemon/dnshandler.h \
				daemon/xfrhandler.c daemon/xfrhandler.h \
				daemon/engine.c daemon/engine.h \
				daemon/signertasks.c daemon/signertasks.h \
				parser/addnsparser.c parser/addnsparser.h \
				parser/signconfparser.c parser/signconfparser.h \
				parser/zonelistparser.c parser/zonelistparser.h \
				hsm.c hsm.h \
				signer/keys.c signer/keys.h \
				signer/nsec3params.c signer/nsec3params.h \
				signer/signconf.c signer/signconf.h \
				signer/stats.c signer/stats.h \
				signer/tools.c signer/tools.h \
				signer/zone.c signer/zone.h \
				signer/zonelist.c signer/zonelist.h \
				signer/backup.c \
				wire/acl.c wire/acl.h \
				wire/axfr.c wire/axfr.h \
				wire/buffer.c wire/buffer.h \
				wire/edns.c wire/edns.h \
				wire/listener.c wire/listener.h \
				wire/netio.c wire/netio.h \
				wire/notify.c wire/notify.h \
				wire/query.c wire/query.h \
				wire/sock.c wire/sock.h \
				wire/tcpset.c wire/tcpset.h \
				wire/tsig.c wire/tsig.h \
				wire/tsig-openssl.c wire/tsig-openssl.h \
				wire/xfrd.c wire/xfrd.h \
				views/recordset.c \
				views/index.c \
				views/iterator.c \
				views/iteratorgeneric.c \
				views/table.c \
				views/views.c \
				views/marshalling.c views/marshalling.h \
				views/commitlog.c \
				views/httpd.c views/httpd.h \
				views/ringbuf.h \
				views/rpc.c \
				views/zoneoutput.c \
				views/proto.h \
	views/libut.h views/utarray.h views/uthash.h views/utlist.h \
	views/utmm.h views/utringbuffer.h views/utstring.h views/utvector.h

analyze:
	cd $(srcdir) ; cppcheck --quiet --force --enable=unusedFunction \
	  $(ods_signerd_SOURCES)

ods_signerd_SOURCES=	ods-signerd.c $(SIGNER_NON_MAIN_SOURCES)
ods_signerd_LDADD=		$(LIBHSM)
ods_signerd_LDADD+=		$(LIBCOMPAT)
ods_signerd_LDADD+=		@LDNS_LIBS@ @XML2_LIBS@ @PTHREAD_LIBS@ @RT_LIBS@ @SSL_LIBS@ @C_LIBS@

ods_signer_SOURCES=		ods-signer.c

ods_signer_LDADD=		$(LIBHSM)
ods_signer_LDADD+=		$(LIBCOMPAT)
ods_signer_LDADD+=		@LDNS_LIBS@ @XML2_LIBS@ @READLINE_LIBS@

LIBCMOCKA = -lcmocka
check_PROGRAMS = test/signer_tests
TESTS = test/signer_tests
test_signer_tests_SOURCES=   test/cmocka/framework/main.c $(SIGNER_NON_MAIN_SOURCES)
test_signer_tests_SOURCES+=  test/cmocka/framework/test_framework.c
test_signer_tests_SOURCES+=  test/cmocka/framework/mock_logging.c
test_signer_tests_SOURCES+=  test/cmocka/framework/mock_core_services.c
test_signer_tests_SOURCES+=  test/cmocka/framework/mock_engine.c
test_signer_tests_SOURCES+=  test/cmocka/framework/mock_hsm.c
test_signer_tests_SOURCES+=  test/cmocka/framework/mock_inputs.c
test_signer_tests_SOURCES+=  test/cmocka/framework/mock_outputs.c
test_signer_tests_SOURCES+=  test/cmocka/framework/mock_worker.c
test_signer_tests_SOURCES+=  test/cmocka/framework/strings.c
test_signer_tests_SOURCES+=  test/cmocka/tests/poc_namedb_update_serial_test.c
test_signer_tests_SOURCES+=  test/cmocka/tests/poc_e2e_test.c
test_signer_tests_SOURCES+=  cryptoki_compat/pkcs11.h
test_signer_tests_CPPFLAGS=  $(AM_CPPFLAGS)
test_signer_tests_CPPFLAGS+= -DTESTING_WITH_MOCKS
test_signer_tests_CPPFLAGS+= -I$(top_srcdir)/signer/src/test/cmocka/framework
test_signer_tests_CPPFLAGS+= -I$(top_srcdir)/libhsm/src/lib/cryptoki_compat
test_signer_tests_LDADD=     $(ods_signerd_LDADD) $(LIBCMOCKA)
# Override functionality that must be handled differently or disabled when testing
test_signer_tests_LDFLAGS=   -Wl,--wrap=privdrop
test_signer_tests_LDFLAGS+=  -Wl,--wrap=pthread_mutex_init
test_signer_tests_LDFLAGS+=  -Wl,--wrap=pthread_mutex_lock
test_signer_tests_LDFLAGS+=  -Wl,--wrap=pthread_mutex_unlock
test_signer_tests_LDFLAGS+=  -Wl,--wrap=pthread_cond_init
test_signer_tests_LDFLAGS+=  -Wl,--wrap=time_now
test_signer_tests_LDFLAGS+=  -Wl,--wrap=ods_fatal_exit
test_signer_tests_LDFLAGS+=  -Wl,--wrap=ods_log_verbose
test_signer_tests_LDFLAGS+=  -Wl,--wrap=ods_log_debug
test_signer_tests_LDFLAGS+=  -Wl,--wrap=ods_log_deeebug
test_signer_tests_LDFLAGS+=  -Wl,--wrap=ods_log_info
test_signer_tests_LDFLAGS+=  -Wl,--wrap=ods_log_warning
test_signer_tests_LDFLAGS+=  -Wl,--wrap=ods_log_error
test_signer_tests_LDFLAGS+=  -Wl,--wrap=ods_log_crit
test_signer_tests_LDFLAGS+=  -Wl,--wrap=ods_log_get_level
test_signer_tests_LDFLAGS+=  -Wl,--wrap=ods_fopen
test_signer_tests_LDFLAGS+=  -Wl,--wrap=ods_fclose
test_signer_tests_LDFLAGS+=  -Wl,--wrap=ods_fgetc
test_signer_tests_LDFLAGS+=  -Wl,--wrap=ods_chown
test_signer_tests_LDFLAGS+=  -Wl,--wrap=util_write_pidfile

test_signer_tests_LDFLAGS+=  -Wl,--wrap=logger_configurecls

test_signer_tests_LDFLAGS+=  -Wl,--wrap=hsm_open2
test_signer_tests_LDFLAGS+=  -Wl,--wrap=hsm_close
test_signer_tests_LDFLAGS+=  -Wl,--wrap=hsm_check_context
test_signer_tests_LDFLAGS+=  -Wl,--wrap=hsm_create_context
test_signer_tests_LDFLAGS+=  -Wl,--wrap=hsm_destroy_context
test_signer_tests_LDFLAGS+=  -Wl,--wrap=hsm_get_dnskey
test_signer_tests_LDFLAGS+=  -Wl,--wrap=hsm_get_error
test_signer_tests_LDFLAGS+=  -Wl,--wrap=hsm_attach
test_signer_tests_LDFLAGS+=  -Wl,--wrap=keycache_lookup

test_signer_tests_LDFLAGS+=  -Wl,--wrap=adapter_write
test_signer_tests_LDFLAGS+=  -Wl,--wrap=janitor_thread_create
test_signer_tests_LDFLAGS+=  -Wl,--wrap=schedule_task
test_signer_tests_LDFLAGS+=  -Wl,--wrap=schedule_pop_task
test_signer_tests_LDFLAGS+=  -Wl,--wrap=schedule_unschedule_task
test_signer_tests_LDFLAGS+=  -Wl,--wrap=task_perform
test_signer_tests_LDFLAGS+=  -Wl,--wrap=zone_backup2

test_signer_tests_LDFLAGS+=  -Wl,--wrap=cmdhandler_cleanup
test_signer_tests_LDFLAGS+=  -Wl,--wrap=dnshandler_cleanup
test_signer_tests_LDFLAGS+=  -Wl,--wrap=xfrhandler_cleanup
test_signer_tests_LDFLAGS+=  -Wl,--wrap=engine_config_cleanup

test_signer_tests_LDFLAGS+=  -Wl,--wrap=metastorageget
test_signer_tests_LDFLAGS+=  -Wl,--wrap=metastorageput

# Wrap direct I/O calls to enable interception of views.c and marshalling.c
# direct .state file access.
test_signer_tests_LDFLAGS+=  -Wl,--wrap=open
test_signer_tests_LDFLAGS+=  -Wl,--wrap=openat
test_signer_tests_LDFLAGS+=  -Wl,--wrap=read
test_signer_tests_LDFLAGS+=  -Wl,--wrap=write
test_signer_tests_LDFLAGS+=  -Wl,--wrap=close
test_signer_tests_LDFLAGS+=  -Wl,--wrap=renameat

# The next one is a cmocka function that is too noisy and offers no way to
# silence it, but by wrapping it we can silence it.
test_signer_tests_LDFLAGS+=  -Wl,--wrap=print_message