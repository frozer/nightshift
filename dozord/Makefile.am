AUTOMAKE_OPTS = gnu
bin_PROGRAMS = dozord

dozord_SOURCES = dozord.c answer.c command.c event-stream.c
dozord_LDADD = ../libdozor/libdozor.la -lpthread -lmosquitto
dozord_CFLAGS = -pthread

AM_CPPFLAGS = -I../libdozor

man_MANS = dozord.8

EXTRA_DIST = dozord.8