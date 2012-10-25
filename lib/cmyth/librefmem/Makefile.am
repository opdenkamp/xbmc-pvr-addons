noinst_LTLIBRARIES = librefmem.la

librefmem_la_SOURCES = alloc.c \
                      debug_refmem.c

INCLUDES=-I../include/ -I../libcmyth/

$(LIB): librefmem.la
	cp -f .libs/librefmem.a .
	cp -f .libs/librefmem.la $(LIB)

