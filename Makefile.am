AUTOMAKE_OPTIONS = foreign

CFLAGS = -Wall
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/../../linux-2.6.x/drivers/staging/icc/include/ -I$(top_srcdir)/../../linux-2.6.x/arch/blackfin/mach-bf561/include/

lib_LTLIBRARIES = libmcapi.la
libmcapi_la_SOURCES  = mcapi.c mcapi_trans_stub.c trans_impl/tran_impl_dev.c


