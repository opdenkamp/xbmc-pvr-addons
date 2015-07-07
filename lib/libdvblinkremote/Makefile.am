noinst_LTLIBRARIES = libdvblinkremote.la

libdvblinkremote_la_SOURCES = channel.cpp \
			dvblinkremote.cpp \
			dvblinkremotecommunication.cpp \
			dvblinkremotehttp.cpp \
			epg.cpp \
			generic_response.cpp \
			item_metadata.cpp \
			m3u_playlist.cpp \
			parental_lock.cpp \
			playback_container.cpp \
			playback_item.cpp \
			playback_object.cpp \
			program.cpp \
			recording.cpp \
			recording_settings.cpp \
			remove_playback_object_request.cpp \
			scheduling.cpp \
			stop_stream_request.cpp \
			stream.cpp \
			stream_request.cpp \
			streaming_capabilities.cpp \
			transcoded_video_stream_request.cpp \
			transcoding_options.cpp \
			util.cpp \
			xml_object_serializer_factory.cpp

LDADD = @abs_top_builddir@/lib/tinyxml2/libtinyxml2.la
 
INCLUDES=-I@abs_srcdir@/..

$(LIB): libdvblinkremote.la
	cp -f .libs/libdvblinkremote.a .
	cp -f .libs/libdvblinkremote.la $(LIB)
