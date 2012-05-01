SUBDIRS = lib addons

zip:
	make -C lib all
	make -C addons zip

install:
	make -C lib all
	make -C addons install

sync:
	cp -f ../xbmc/cores/dvdplayer/DVDDemuxers/DVDDemuxPacket.h xbmc/DVDDemuxPacket.h
	cp -f ../addons/library.xbmc.addon/libXBMC_addon.h         xbmc/libXBMC_addon.h
	cp -f ../addons/library.xbmc.gui/libXBMC_gui.h             xbmc/libXBMC_gui.h
	cp -f ../addons/library.xbmc.pvr/libXBMC_pvr.h             xbmc/libXBMC_pvr.h
	cp -f ../xbmc/addons/include/xbmc_addon_cpp_dll.h          xbmc/xbmc_addon_cpp_dll.h
	cp -f ../xbmc/addons/include/xbmc_addon_dll.h              xbmc/xbmc_addon_dll.h
	cp -f ../xbmc/addons/include/xbmc_addon_types.h            xbmc/xbmc_addon_types.h
	cp -f ../xbmc/addons/include/xbmc_pvr_dll.h                xbmc/xbmc_pvr_dll.h
	cp -f ../xbmc/addons/include/xbmc_pvr_types.h              xbmc/xbmc_pvr_types.h
	cp -f ../xbmc/addons/include/xbmc_scr_dll.h                xbmc/xbmc_scr_dll.h
	cp -f ../xbmc/addons/include/xbmc_scr_types.h              xbmc/xbmc_scr_types.h
	cp -f ../xbmc/addons/include/xbmc_vis_dll.h                xbmc/xbmc_vis_dll.h
	cp -f ../xbmc/addons/include/xbmc_vis_types.h              xbmc/xbmc_vis_types.h
	
