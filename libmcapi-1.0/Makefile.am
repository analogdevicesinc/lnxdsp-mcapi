AUTOMAKE_OPTIONS = foreign

CFLAGS = -Wall
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/../../../linux-2.6.x/drivers/staging/icc/include/ -I$(top_srcdir)/../../../linux-2.6.x/arch/blackfin/mach-bf561/include/
lib_LTLIBRARIES = libmcapi.la
library_includedir = /usr/include/mcapi-1.0
library_include_HEADERS = include/mcapi_datatypes.h  include/mcapi_dev_impl.h  include/mcapi.h  include/mcapi_test.h  include/mcapi_trans.h  include/transport_sm.h
libmcapi_la_SOURCES  = mcapi.c mcapi_trans_stub.c trans_impl/tran_impl_dev.c

