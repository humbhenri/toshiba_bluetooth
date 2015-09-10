#
# Makefile -- makefile for the Toshiba Bluetooth driver
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2, or (at your option) any
# later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# Written by Soós Péter <sp@osb.hu>, 2002-2004
# Modified by Azael Avalos <coproscefalo@gmail.com>, 2013, 2015
#

MODULE_NAME	= toshiba_bluetooth

ifeq ($(KERNELRELEASE),)

DESTDIR	= 
MODDIR	= $(DESTDIR)/lib/modules
KVERS	= $(shell uname -r)
KVER	= $(KVERS)
VMODDIR = $(MODDIR)/$(KVER)
INSTDIR	= kernel/drivers/platform/x86
#KSRC	= /usr/src/linux
KSRC	= $(VMODDIR)/build
KMODDIR	= $(KSRC)/drivers/platform/x86
KDOCDIR	= $(KSRC)/Documentation/ABI/testing
PWD	= $(shell pwd)
TODAY	= $(shell date +%Y%m%d)
KERNEL	= $(shell echo $(KVER) | cut -d . -f 1-2)

DEPMOD	= /sbin/depmod -a
RMMOD	= /sbin/modprobe -r
INSMOD	= /sbin/modprobe
INSTALL	= install -m 644
MKDIR	= mkdir -p
RM	= rm -f
FIND	= find
endif

obj-m         += $(MODULE_NAME).o

all:		 $(MODULE_NAME).ko

clean:
		make -C $(KSRC) M=$(PWD) clean
		$(RM) -r *~ "#*#" .swp
		$(RM) -r Module.symvers Modules.symvers

install:	all
		# Removing module from locations used by previous versions
		$(RM) $(VMODDIR)/kernel/drivers/platform/x86/$(MODULE_NAME).ko
		make INSTALL_MOD_PATH=$(DESTDIR) INSTALL_MOD_DIR=$(INSTDIR) -C $(KSRC) M=$(PWD) modules_install

unload:
		$(RMMOD) $(MODULE_NAME) || :

load:		install unload
		$(DEPMOD)
		$(INSMOD) $(MODULE_NAME)

uninstall:	unload
		$(FIND) $(VMODDIR) -name "$(MODULE_NAME).ko" -exec $(RM) {} \;
		$(DEPMOD)

$(MODULE_NAME).ko:
		$(MAKE) -C $(KSRC) SUBDIRS=$(PWD) modules

kinstall:
		$(RM) -r $(KMODDIR)
		$(MKDIR) $(KMODDIR)
		$(INSTALL) *.h *.c sections.lds $(KMODDIR)
		$(MKDIR) $(KDOCDIR)
		$(INSTALL) doc/README $(KDOCDIR)

# End of file