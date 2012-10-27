SUBDIRS = librefmem libcmyth

noinst_LTLIBRARIES = libcmyth.la

libcmyth_la_SOURCES =
libcmyth_la_LIBADD = libcmyth/libcmyth.la librefmem/librefmem.la

$(LIB): libcmyth.la
	cp -f .libs/libcmyth.a .
	cp -f .libs/libcmyth.la $(LIB)

