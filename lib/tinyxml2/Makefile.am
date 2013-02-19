noinst_LTLIBRARIES = libtinyxml2.la

libtinyxml2_la_SOURCES = tinyxml2.cpp \
			xmltest.cpp
$(LIB): libtinyxml2.la
	cp -f .libs/libtinyxml2.a .
	cp -f .libs/libtinyxml2.la $(LIB)