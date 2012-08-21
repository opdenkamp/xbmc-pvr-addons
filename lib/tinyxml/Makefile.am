noinst_LTLIBRARIES = libtinyxml.la

libtinyxml_la_SOURCES = tinystr.cpp \
                    tinyxml.cpp \
                    tinyxmlerror.cpp \
                    tinyxmlparser.cpp \
                    XMLUtils.cpp

$(LIB): libtinyxml.la
	cp -f .libs/libtinyxml.a .
	cp -f .libs/libtinyxml.la $(LIB)

