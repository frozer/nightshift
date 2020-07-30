AUTOMAKE_OPTS = gnu
bin_PROGRAMS = dozord

dozord_SOURCES = dozord.c answer.c command.c
dozord_LDADD = ../libdozor/libdozor.la -lpthread -lmosquitto
dozord_CFLAGS = -pthread

include_HEADERS = answer.h command.h nightshift-mqtt.h

AM_CPPFLAGS = -I../libdozor

man_MANS = dozord.8

EXTRA_DIST = dozord.8