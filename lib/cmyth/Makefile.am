SUBDIRS = librefmem libcmyth

noinst_LTLIBRARIES = libcmyth.la

if HOST_IS_OSX
AM_LDFLAGS = -lmysqlclient -lpthread -lz -lm -lssl -lcrypto -ldl
else
AM_LDFLAGS = -lmysqlclient -lpthread -lz -lm -lrt -lssl -lcrypto -ldl
endif

libcmyth_la_SOURCES =
libcmyth_la_LIBADD = libcmyth/libcmyth.la librefmem/librefmem.la

$(LIB): libcmyth.la
	cp -f .libs/libcmyth.a .
	cp -f .libs/libcmyth.la $(LIB)

