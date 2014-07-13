#
# (C) Copyright 2009 Texas Instruments.
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

include $(TOPDIR)/config.mk

COMMONDIR = ../tomtom/common
PLATDIR = ../tomtom/plat-omap3

LIB	= lib$(BOARD).a

# Boot shell scripts
USHOBJS	:= bootcmd.o altbootcmd.o preboot.o preboot_plat.o

# U-Boot 1.1.4 build system sucks :P
ifeq (,$(shell fgrep -q 'CONFIG_DEBUG_BUILD' \
	$(TOPDIR)/include/`sed -ne 's|.*<\(.*\)>.*|\1|p' $(TOPDIR)/include/config.h` \
	2>/dev/null || echo x))
USHOBJS	+= bootcmd_debug.o altbootcmd_debug.o preboot_debug.o
endif

OBJS	:= \
	santiago.o \
	sys_info.o \

OBJS += $(COMMONDIR)/bootcount.o $(COMMONDIR)/env_init.o
OBJS += $(PLATDIR)/boot_mode.o $(PLATDIR)/flipflop.o $(PLATDIR)/watchdog.o

$(LIB):	$(OBJS) $(USHOBJS)
	$(AR) crv $@ $^

%.o: %.image
	cd $(<D); \
	$(OBJCOPY) -I binary -O elf32-littlearm -B arm \
		--rename-section .data=.bootscript $(<F) $(@F)

.SECONDARY: $(USHOBJS:.o=.image)

%.image: %.ush Makefile
	$(TOPDIR)/tools/mkimage -A arm -O linux -T script -C none \
		-a 0 -e 0 -n $(*F) -d $< $@.tmp
	-dd if=$@.tmp bs=4 of=$@ conv=sync
	$(RM) $@.tmp

clean:
	rm -f $(OBJS) $(USHOBJS)

distclean:	clean
	rm -f $(LIB) core *.bak .depend

#########################################################################

.depend:	Makefile $(OBJS:.o=.c)
		$(CC) -M $(CPPFLAGS) $(OBJS:.o=.c) > $@

-include .depend

#########################################################################
