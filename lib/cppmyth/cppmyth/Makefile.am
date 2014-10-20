noinst_LTLIBRARIES = libcppmyth.la
#lib_LTLIBRARIES    = libcppmyth.la

AM_CPPFLAGS = -fPIC -Wall -Wextra \
	-I@abs_srcdir@/src \
	-I@abs_top_srcdir@/jansson/src

libcppmyth_la_SOURCES = \
	src/private/mythdto/mythdto.cpp \
	src/private/builtin.c \
	src/private/mythsocket.cpp \
	src/private/mythjsonparser.cpp \
	src/private/mythjsonbinder.cpp \
	src/private/mythwscontent.cpp \
	src/private/mythwsrequest.cpp \
	src/private/mythwsresponse.cpp \
	src/private/uriparser.cpp \
	src/proto/mythprotobase.cpp \
	src/proto/mythprotoevent.cpp \
	src/proto/mythprotomonitor.cpp \
	src/proto/mythprotoplayback.cpp \
	src/proto/mythprotorecorder.cpp \
	src/proto/mythprototransfer.cpp \
	src/mythcontrol.cpp \
	src/mythdebug.cpp \
	src/mytheventhandler.cpp \
	src/mythfileplayback.cpp \
	src/mythlivetvplayback.cpp \
	src/mythrecordingplayback.cpp \
	src/mythtypes.cpp \
	src/mythwsapi.cpp \
	src/mythwsstream.cpp
