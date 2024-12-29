AUTOMAKE_OPTIONS = gnu
lib_LTLIBRARIES = libdozor.la

libdozor_la_SOURCES = libdozor.c device-event.c dozor-crypto.c rc4.c utils.c event.c dozor-crypto.h event.h device-event.h rc4.h session.h utils.h
libdozor_la_LIBADD = $(top_builddir)/liblogger/liblogger.la
libdozor_la_CPPFLAGS = -I$(top_srcdir)/liblogger
include_HEADERS = dozor.h

libdozor_la_LDFLAGS = -version-info 0:0:0