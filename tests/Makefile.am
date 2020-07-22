INCLUDES                = -I$(top_srcdir)/include


#bin_PROGRAMS            = endpoints1 msg1 msg2 pkt1 pkt2 pkt3 scl1 scl2 cces_msg1 bmp2jpg arm_sharc_msg_demo arm_sharc_msg_test arm_sharc_pkt1 arm_sharc_scl1 arm_sharc_audio_vol
bin_PROGRAMS            = endpoints1 msg1 msg2 cces_msg1 bmp2jpg arm_sharc_audio_vol arm_sharc_msg_demo arm_sharc_msg_test

endpoints1_SOURCES         = endpoints1.c
endpoints1_LDADD           = $(top_builddir)/libmcapi.la

msg1_SOURCES        = msg1.c
msg1_LDADD          = $(top_builddir)/libmcapi.la

bmp2jpg_SOURCES        = bmp2jpg.c
bmp2jpg_LDADD          = $(top_builddir)/libmcapi.la

msg2_SOURCES        = msg2.c
msg2_LDADD          = $(top_builddir)/libmcapi.la

pkt1_SOURCES    = pkt1.c
pkt1_LDADD      = $(top_builddir)/libmcapi.la

pkt2_SOURCES    = pkt2.c
pkt2_LDADD      = $(top_builddir)/libmcapi.la

pkt3_SOURCES    = pkt3.c
pkt3_LDADD      = $(top_builddir)/libmcapi.la

scl1_SOURCES    = scl1.c
scl1_LDADD      = $(top_builddir)/libmcapi.la

scl2_SOURCES    = scl2.c
scl2_LDADD      = $(top_builddir)/libmcapi.la

cces_msg1_SOURCES    = cces_msg1.c
cces_msg1_LDADD      = $(top_builddir)/libmcapi.la

arm_sharc_msg_demo_LDFLAGS    = -lpthread
arm_sharc_msg_demo_SOURCES    = arm_sharc_msg_demo.c
arm_sharc_msg_demo_LDADD      = $(top_builddir)/libmcapi.la

arm_sharc_msg_test_LDFLAGS    = -lpthread
arm_sharc_msg_test_SOURCES    = arm_sharc_msg_test.c
arm_sharc_msg_test_LDADD      = $(top_builddir)/libmcapi.la

arm_sharc_scl1_SOURCES    = arm_sharc_scl1.c
arm_sharc_scl1_LDADD      = $(top_builddir)/libmcapi.la

arm_sharc_pkt1_SOURCES    = arm_sharc_pkt1.c
arm_sharc_pkt1_LDADD      = $(top_builddir)/libmcapi.la

arm_sharc_audio_vol_SOURCES    = arm_sharc_audio_vol.c
arm_sharc_audio_vol_LDADD      = $(top_builddir)/libmcapi.la
