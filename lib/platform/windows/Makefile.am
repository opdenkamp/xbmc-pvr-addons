noinst_LTLIBRARIES = libpvrwindows.la

libpvrwindows_la_SOURCES = dlfcn-win32.cpp os-threads.cpp

$(LIB): libpvrwindows.la
	cp -f .libs/libpvrwindows.a .
	cp -f .libs/libpvrwindows.la $(LIB)
