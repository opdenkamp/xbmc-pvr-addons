noinst_LTLIBRARIES = libcmyth.la

libcmyth_la_SOURCES = bookmark.c \
                     commbreak.c \
                     connection.c \
                     event.c \
                     file.c \
                     freespace.c \
                     input.c \
                     keyframe.c \
                     livetv.c \
                     mysql_query.c \
                     mythtv_mysql.c \
                     posmap.c \
                     proginfo.c \
                     proglist.c \
                     rec_num.c \
                     recorder.c \
                     ringbuf.c \
                     socket.c \
                     timestamp.c \
                     debug.c \
                     utf8tolatin1.c

INCLUDES=-I../include/ $(MYSQL_INCLUDES)

AM_LDFLAGS = $(MYSQL_LIBS)

$(LIB): libcmyth.la
	cp -f .libs/libcmyth.a .
	cp -f .libs/libcmyth.la $(LIB)

