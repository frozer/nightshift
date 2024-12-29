AUTOMAKE_OPTS = gnu
bin_PROGRAMS = parser

parser_SOURCES = parser.c
parser_LDADD = $(top_builddir)/liblogger/liblogger.la $(top_builddir)/libdozor/libdozor.la
parser_CPPFLAGS = -I$(top_srcdir)/liblogger -I$(top_srcdir)/libdozor