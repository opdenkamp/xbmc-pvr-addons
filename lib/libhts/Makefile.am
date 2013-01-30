noinst_LTLIBRARIES = libhts.la

libhts_la_SOURCES = htsmsg.c \
                    htsmsg_binary.c \
                    htsbuf.c \
                    htsstr.c \
                    sha1.c

if !HOST_IS_WINDOWS
  libhts_la_SOURCES += net_posix.c
endif

libhts_la_CPPFLAGS=-std=c99 -D_GNU_SOURCE

$(LIB): libhts.la
	cp -f .libs/libhts.a .
	cp -f .libs/libhts.la $(LIB)

