## $Id: Makefile.am,v 5.37 2010/03/21 11:57:49 lirc Exp $

## Process this file with automake to produce Makefile.in 

## here the correct driver is substituted in by the configure script
## (or empty for none)

EXTRA_DIST = lirc.h kcompat.h Makefile.kernel Makefile.common

DIST_SUBDIRS = \
	lirc_dev \
	lirc_atiusb \
	lirc_bt829 \
	lirc_gpio \
	lirc_i2c \
	lirc_igorplugusb \
	lirc_imon \
	lirc_parallel \
	lirc_sasem \
	lirc_serial \
	lirc_sir \
	lirc_ttusbir \
	lirc_wpc8769l

SUBDIRS = @lirc_driver@

CLEANFILES = *~

