AUTOMAKE_OPTS = gnu
bin_PROGRAMS = dozord

dozord_SOURCES = main.c nightshift-mqtt.c socket-server.c command.c logger.c app-config.c command.h nightshift-mqtt.h logger.h socket-server.h app-config.h
dozord_LDADD = ../libdozor/libdozor.la -lpthread -lmosquitto
dozord_CFLAGS = -pthread
AM_CPPFLAGS = -I../libdozor

man_MANS = dozord.8

EXTRA_DIST = dozord.8