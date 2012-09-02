noinst_LTLIBRARIES = libjsoncpp.la

libjsoncpp_la_SOURCES = src/lib_json/json_reader.cpp \
                        src/lib_json/json_value.cpp \
                        src/lib_json/json_writer.cpp

INCLUDES=-Iinclude/

$(LIB): libjsoncpp.la
	cp -f .libs/libjsoncpp.a .
	cp -f .libs/libjsoncpp.la $(LIB)

