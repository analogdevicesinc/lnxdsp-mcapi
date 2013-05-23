VER = libmcapi-1.0

CFLAGS += -I$(ROOTDIR)/linux-2.6.x/drivers/staging/icc/include -I$(ROOTDIR)/linux-2.6.x/arch/blackfin/mach-bf561/include/mach

include $(ROOTDIR)/tools/autotools.mk

$(BUILDDIR)/Makefile: $(VER)/configure

$(VER)/configure: $(VER)/aclocal.m4 $(VER)/configure.ac
	cd $(VER) && ./autogen.sh


