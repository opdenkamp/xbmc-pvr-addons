if ADDON_MYTHTV
          ADDITIONAL_SUBDIRS = pvr.mythtv.cmyth
endif

SUBDIRS = pvr.demo \
          pvr.dvbviewer \
          pvr.hts \
          pvr.mediaportal.tvserver \
          pvr.nextpvr \
          pvr.vdr.vnsi \
          pvr.njoy \
          pvr.vuplus \
          pvr.fortherecord.argus \
          $(ADDITIONAL_SUBDIRS)

clean:
	-rm -f *.zip
	for d in $(SUBDIRS); do if test -f $$d/Makefile; then $(MAKE) -C $$d clean; fi; done

zip:
	for d in $(SUBDIRS); do if test -f $$d/Makefile; then $(MAKE) -C $$d zip; fi; done

install:
	for d in $(SUBDIRS); do if test -f $$d/Makefile; then $(MAKE) -C $$d install; fi; done

