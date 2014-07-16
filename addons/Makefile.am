if ADDON_IPTVSIMPLE
          ADDON_IPTVSIMPLE_SUBDIRS = pvr.iptvsimple
endif
if ADDON_MYTHTV
          ADDON_MYTHTV_SUBDIRS = pvr.mythtv.cmyth
endif

### Keep this in alphabetical order
SUBDIRS = pvr.argustv \
          pvr.demo \
          pvr.dvblink \
          pvr.dvbviewer \
          pvr.hts \
          pvr.mediaportal.tvserver \
          pvr.nextpvr \
          pvr.njoy \
          pvr.vdr.vnsi \
          pvr.vuplus \
          pvr.wmc \
          $(ADDON_IPTVSIMPLE_SUBDIRS) \
          $(ADDON_MYTHTV_SUBDIRS)

clean:
	-rm -f *.zip
	for d in $(SUBDIRS); do if test -f $$d/Makefile; then $(MAKE) -C $$d clean; fi; done

zip:
	for d in $(SUBDIRS); do if test -f $$d/Makefile; then $(MAKE) -C $$d zip; fi; done

install:
	for d in $(SUBDIRS); do if test -f $$d/Makefile; then $(MAKE) -C $$d install; fi; done

