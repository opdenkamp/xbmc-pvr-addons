noinst_LTLIBRARIES = libhts.la

libhts_la_SOURCES = htsmsg.c \
                    htsmsg_binary.c \
                    htsbuf.c \
                    htsstr.c \
                    net_posix.c \
                    sha1.c

libhts_la_CPPFLAGS=-std=c99 -D_GNU_SOURCE

$(LIB): libhts.la
	cp -f .libs/libhts.a .
	cp -f .libs/libhts.la $(LIB)

