VER = libmcapi-1.0

include $(ROOTDIR)/tools/autotools.mk

$(VER)/configure: $(VER)/aclocal.m4
	cd $(VER) && $(AUTOCONF)


