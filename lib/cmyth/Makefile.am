SUBDIRS = librefmem libcmyth

lib_LTLIBRARIES = libcmyth.la

AM_LDFLAGS = -lmysqlclient -lpthread -lz -lm -lrt -lssl -lcrypto -ldl

libcmyth_la_SOURCES =
libcmyth_la_LIBADD = libcmyth/libcmyth.la librefmem/librefmem.la

$(LIB): libcmyth.la
	cp -f .libs/libcmyth.a .
	cp -f .libs/libcmyth.la $(LIB)
	cp -f .libs/libcmyth.so* $(LIB)

