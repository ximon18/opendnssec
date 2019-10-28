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
bin_PROGRAMS =  ods-getconf

ods_signerd_SOURCES=		ods-signerd.c \
				adapter/adapi.c adapter/adapi.h \
				adapter/adapter.c adapter/adapter.h \
				adapter/addns.c adapter/addns.h \
				adapter/adfile.c adapter/adfile.h \
				adapter/adutil.c adapter/adutil.h \
				daemon/cfg.c daemon/cfg.h \
				daemon/cmdhandler.c daemon/cmdhandler.h \
				daemon/dnshandler.c daemon/dnshandler.h \
				daemon/xfrhandler.c daemon/xfrhandler.h \
				daemon/engine.c daemon/engine.h \
				daemon/signal.c daemon/signal.h \
				daemon/worker.c daemon/worker.h \
				parser/addnsparser.c parser/addnsparser.h \
				parser/confparser.c parser/confparser.h \
				parser/signconfparser.c parser/signconfparser.h \
				parser/zonelistparser.c parser/zonelistparser.h \
				scheduler/fifoq.c scheduler/fifoq.h \
				scheduler/schedule.c scheduler/schedule.h \
				scheduler/task.c scheduler/task.h \
				shared/allocator.c shared/allocator.h \
				shared/duration.c shared/duration.h \
				shared/file.c shared/file.h \
				shared/hsm.c shared/hsm.h \
				shared/locks.c shared/locks.h \
				shared/log.c shared/log.h \
				shared/privdrop.c shared/privdrop.h \
				shared/status.c shared/status.h \
				shared/util.c shared/util.h \
				signer/backup.c signer/backup.h \
				signer/denial.c signer/denial.h \
				signer/domain.c signer/domain.h \
				signer/ixfr.c signer/ixfr.h \
				signer/keys.c signer/keys.h \
				signer/namedb.c signer/namedb.h \
				signer/nsec3params.c signer/nsec3params.h \
				signer/rrset.c signer/rrset.h \
				signer/signconf.c signer/signconf.h \
				signer/stats.c signer/stats.h \
				signer/tools.c signer/tools.h \
				signer/zone.c signer/zone.h \
				signer/zonelist.c signer/zonelist.h \
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
				wire/xfrd.c wire/xfrd.h

ods_signerd_LDADD=		$(LIBHSM)
ods_signerd_LDADD+=		$(LIBCOMPAT)
ods_signerd_LDADD+=		@LDNS_LIBS@ @XML2_LIBS@ @PTHREAD_LIBS@ @RT_LIBS@ @SSL_LIBS@ @C_LIBS@

SIGNER_NON_MAIN_SOURCES = adapter/adapi.c adapter/adapi.h \
				adapter/adapter.c adapter/adapter.h \
				adapter/addns.c adapter/addns.h \
				adapter/adfile.c adapter/adfile.h \
				adapter/adutil.c adapter/adutil.h \
				daemon/cfg.c daemon/cfg.h \
				daemon/cmdhandler.c daemon/cmdhandler.h \
				daemon/dnshandler.c daemon/dnshandler.h \
				daemon/xfrhandler.c daemon/xfrhandler.h \
				daemon/engine.c daemon/engine.h \
				daemon/signal.c daemon/signal.h \
				daemon/worker.c daemon/worker.h \
				parser/addnsparser.c parser/addnsparser.h \
				parser/confparser.c parser/confparser.h \
				parser/signconfparser.c parser/signconfparser.h \
				parser/zonelistparser.c parser/zonelistparser.h \
				scheduler/fifoq.c scheduler/fifoq.h \
				scheduler/schedule.c scheduler/schedule.h \
				scheduler/task.c scheduler/task.h \
				shared/allocator.c shared/allocator.h \
				shared/duration.c shared/duration.h \
				shared/file.c shared/file.h \
				shared/hsm.c shared/hsm.h \
				shared/locks.c shared/locks.h \
				shared/log.c shared/log.h \
				shared/privdrop.c shared/privdrop.h \
				shared/status.c shared/status.h \
				shared/util.c shared/util.h \
				signer/backup.c signer/backup.h \
				signer/denial.c signer/denial.h \
				signer/domain.c signer/domain.h \
				signer/ixfr.c signer/ixfr.h \
				signer/keys.c signer/keys.h \
				signer/namedb.c signer/namedb.h \
				signer/nsec3params.c signer/nsec3params.h \
				signer/rrset.c signer/rrset.h \
				signer/signconf.c signer/signconf.h \
				signer/stats.c signer/stats.h \
				signer/tools.c signer/tools.h \
				signer/zone.c signer/zone.h \
				signer/zonelist.c signer/zonelist.h \
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
				wire/xfrd.c wire/xfrd.h

ods_signer_SOURCES=		ods-signer.c $(SIGNER_NON_MAIN_SOURCES)
ods_signer_LDADD=		$(LIBHSM)
ods_signer_LDADD+=		@LDNS_LIBS@ @XML2_LIBS@ @RT_LIBS@ @SSL_LIBS@ 
ods_signer_LDADD+=		$(LIBCOMPAT)

ods_getconf_SOURCES=		ods-getconf.c \
				adapter/adapi.c adapter/adapi.h \
				adapter/adapter.c adapter/adapter.h \
				adapter/addns.c adapter/addns.h \
				adapter/adfile.c adapter/adfile.h \
				adapter/adutil.c adapter/adutil.h \
				daemon/cfg.c daemon/cfg.h \
				daemon/cmdhandler.c daemon/cmdhandler.h \
				daemon/dnshandler.c daemon/dnshandler.h \
				daemon/xfrhandler.c daemon/xfrhandler.h \
				daemon/engine.c daemon/engine.h \
				daemon/signal.c daemon/signal.h \
				daemon/worker.c daemon/worker.h \
				parser/addnsparser.c parser/addnsparser.h \
				parser/confparser.c parser/confparser.h \
				parser/signconfparser.c parser/signconfparser.h \
				parser/zonelistparser.c parser/zonelistparser.h \
				scheduler/fifoq.c scheduler/fifoq.h \
				scheduler/schedule.c scheduler/schedule.h \
				scheduler/task.c scheduler/task.h \
				shared/allocator.c shared/allocator.h \
				shared/duration.c shared/duration.h \
				shared/file.c shared/file.h \
				shared/hsm.c shared/hsm.h \
				shared/locks.c shared/locks.h \
				shared/log.c shared/log.h \
				shared/privdrop.c shared/privdrop.h \
				shared/status.c shared/status.h \
				shared/util.c shared/util.h \
				signer/backup.c signer/backup.h \
				signer/denial.c signer/denial.h \
				signer/domain.c signer/domain.h \
				signer/ixfr.c signer/ixfr.h \
				signer/keys.c signer/keys.h \
				signer/namedb.c signer/namedb.h \
				signer/nsec3params.c signer/nsec3params.h \
				signer/rrset.c signer/rrset.h \
				signer/signconf.c signer/signconf.h \
				signer/stats.c signer/stats.h \
				signer/tools.c signer/tools.h \
				signer/zone.c signer/zone.h \
				signer/zonelist.c signer/zonelist.h \
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
				wire/xfrd.c wire/xfrd.h

ods_getconf_LDADD=		$(LIBHSM)
ods_getconf_LDADD+=		@SSL_LIBS@ @LDNS_LIBS@ @XML2_LIBS@ @RT_LIBS@
ods_getconf_LDADD+=		$(LIBCOMPAT)

LIBCMOCKA = -lcmocka
check_PROGRAMS = test/namedb_test
TESTS = test/namedb_test
test_namedb_test_SOURCES=   signer/test/cmocka/main.c $(SIGNER_NON_MAIN_SOURCES)
test_namedb_test_SOURCES+=  signer/test/cmocka/test_framework.c
test_namedb_test_SOURCES+=  signer/test/cmocka/mock_logging.c
test_namedb_test_SOURCES+=  signer/test/cmocka/mock_core_services.c
test_namedb_test_SOURCES+=  signer/test/cmocka/mock_hsm.c
test_namedb_test_SOURCES+=  signer/test/cmocka/mock_inputs.c
test_namedb_test_SOURCES+=  signer/test/cmocka/mock_outputs.c
test_namedb_test_SOURCES+=  signer/test/cmocka/mock_worker.c
test_namedb_test_SOURCES+=  signer/test/cmocka/poc_e2e_test.c
test_namedb_test_SOURCES+=  cryptoki_compat/pkcs11.h
test_namedb_test_CPPFLAGS=  $(AM_CPPFLAGS)
test_namedb_test_CPPFLAGS+= -DTESTING_WITH_MOCKS
test_namedb_test_CPPFLAGS+= -I$(top_srcdir)/signer/src/signer/test/cmocka
test_namedb_test_CPPFLAGS+= -I$(top_srcdir)/libhsm/src/lib/cryptoki_compat
test_namedb_test_LDADD=		$(ods_signer_LDADD) $(LIBCMOCKA)
# Override functionality that must be handled differently or disabled when testing
test_namedb_test_LDFLAGS=	-Wl,--wrap=ods_fatal_exit
test_namedb_test_LDFLAGS+=  -Wl,--wrap=ods_log_verbose
test_namedb_test_LDFLAGS+=  -Wl,--wrap=ods_log_debug
test_namedb_test_LDFLAGS+=  -Wl,--wrap=ods_log_deeebug
test_namedb_test_LDFLAGS+=  -Wl,--wrap=ods_log_info
test_namedb_test_LDFLAGS+=  -Wl,--wrap=ods_log_warning
test_namedb_test_LDFLAGS+=  -Wl,--wrap=ods_log_error
test_namedb_test_LDFLAGS+=  -Wl,--wrap=ods_log_crit
test_namedb_test_LDFLAGS+=  -Wl,--wrap=ods_log_get_level
test_namedb_test_LDFLAGS+=  -Wl,--wrap=time_now
test_namedb_test_LDFLAGS+=  -Wl,--wrap=hsm_check_context
test_namedb_test_LDFLAGS+=  -Wl,--wrap=hsm_create_context
test_namedb_test_LDFLAGS+=  -Wl,--wrap=hsm_destroy_context
test_namedb_test_LDFLAGS+=  -Wl,--wrap=hsm_get_dnskey
test_namedb_test_LDFLAGS+=  -Wl,--wrap=hsm_get_error
test_namedb_test_LDFLAGS+=  -Wl,--wrap=keylookup
test_namedb_test_LDFLAGS+=  -Wl,--wrap=pthread_mutex_lock
test_namedb_test_LDFLAGS+=  -Wl,--wrap=pthread_mutex_unlock
test_namedb_test_LDFLAGS+=  -Wl,--wrap=adapter_write
test_namedb_test_LDFLAGS+=  -Wl,--wrap=zone_backup2
test_namedb_test_LDFLAGS+=  -Wl,--wrap=ods_fopen
test_namedb_test_LDFLAGS+=  -Wl,--wrap=ods_fclose
test_namedb_test_LDFLAGS+=  -Wl,--wrap=ods_fgetc
# The next one is a cmocka function that is too noisy and offers no way to
# silence it, but by wrapping it we can silence it.
test_namedb_test_LDFLAGS+=  -Wl,--wrap=print_message