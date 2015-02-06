if ADDON_ARGUSTV
	 ADDON_ARGUSTV_SUBDIRS = pvr.argustv
endif

if ADDON_DEMO
	 ADDON_DEMO_SUBDIRS = pvr.demo
endif

if ADDON_DVBLINK
	 ADDON_DVBLINK_SUBDIRS = pvr.dvblink
endif

if ADDON_DVBVIEWER
	 ADDON_DVBVIEWER_SUBDIRS = pvr.dvbviewer
endif

if ADDON_FILMON
	 ADDON_FILMON_SUBDIRS = pvr.filmon
endif

if ADDON_HTS
	 ADDON_HTS_SUBDIRS = pvr.hts
endif

if ADDON_IPTVSIMPLE
	 ADDON_IPTVSIMPLE_SUBDIRS = pvr.iptvsimple
endif

if ADDON_MEDIAPORTAL_TVSERVER
	 ADDON_MEDIAPORTAL_TVSERVER_SUBDIRS = pvr.mediaportal.tvserver
endif

if ADDON_MYTHTV
	 ADDON_MYTHTV_SUBDIRS = pvr.mythtv
endif

if ADDON_NEXTPVR
	 ADDON_NEXTPVR_SUBDIRS = pvr.nextpvr
endif

if ADDON_NJOY
	 ADDON_NJOY_SUBDIRS = pvr.njoy
endif

if ADDON_VDR_VNSI
	 ADDON_VDR_VNSI_SUBDIRS = pvr.vdr.vnsi
endif

if ADDON_VUPLUS
	 ADDON_VUPLUS_SUBDIRS = pvr.vuplus
endif

if ADDON_WMC
	 ADDON_WMC_SUBDIRS = pvr.wmc
endif

### Keep this in alphabetical order
SUBDIRS = $(ADDON_ARGUSTV_SUBDIRS) \
	  $(ADDON_DEMO_SUBDIRS) \
	  $(ADDON_DVBLINK_SUBDIRS) \
	  $(ADDON_DVBVIEWER_SUBDIRS) \
	  $(ADDON_FILMON_SUBDIRS) \
	  $(ADDON_HTS_SUBDIRS) \
	  $(ADDON_IPTVSIMPLE_SUBDIRS) \
	  $(ADDON_MEDIAPORTAL_TVSERVER_SUBDIRS) \
	  $(ADDON_MYTHTV_SUBDIRS) \
	  $(ADDON_NEXTPVR_SUBDIRS) \
	  $(ADDON_NJOY_SUBDIRS) \
	  $(ADDON_VDR_VNSI_SUBDIRS) \
	  $(ADDON_VUPLUS_SUBDIRS) \
	  $(ADDON_WMC_SUBDIRS)

clean:
	-rm -f *.zip
	for d in $(SUBDIRS); do if test -f $$d/Makefile; then $(MAKE) -C $$d clean; fi; done

zip:
	for d in $(SUBDIRS); do if test -f $$d/Makefile; then $(MAKE) -C $$d zip; fi; done

install:
	for d in $(SUBDIRS); do if test -f $$d/Makefile; then $(MAKE) -C $$d install; fi; done

