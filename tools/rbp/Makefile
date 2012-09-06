include Makefile.include
include depends.mk

SOURCE=../../

ifeq ($(USE_BUILDROOT),1)
	export PATH:=$(PREFIX)/bin:$(BUILDROOT)/output/host/usr/bin:$(SYSROOT)/usr/bin:$(PATH)
endif

CONFIGURE=./configure --prefix=$(PREFIX) --build=$(BUILD) --host=$(HOST)

all: configure

clean:
distclean:
	cd $(SOURCE);

configure:
	cd $(SOURCE); ./bootstrap
	cd $(SOURCE); $(CONFIGURE)
	cd $(SOURCE); make -j $(JOBS)
	cd $(SOURCE); make install
	cd $(SOURCE); make zip
