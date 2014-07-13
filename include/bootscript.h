/*
 * Import bootscripts from binary blobs
 * See board/tomtom/... as an example of how this works
 *
 * Copyright (C) 2010 TomTom International B.V.
 * 
 ************************************************************************
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 
 * 02110-1301, USA.
 ************************************************************************
 */

#ifndef __BOOTSCRIPT_H
#define __BOOTSCRIPT_H

#include <common.h>

#if defined(CONFIG_CMD_AUTOSCRIPT) || defined(CFG_CMD_AUTOSCRIPT)
static inline void __set_bootscript_in_env(char *e, void *p)
{
	char t[32];

	sprintf(t, "autoscr %p", p);
	setenv(e, t);
}
# define IMPORT_BOOTSCRIPT(s,v)	\
	extern char _binary_##s##_image_start;	\
	__set_bootscript_in_env(#v,&_binary_##s##_image_start);
#else
# define IMPORT_BOOTSCRIPT(s,v) __IMPORT_BOOTSCRIPT_is_broken__
#endif

#endif /* __BOOTSCRIPT_H */

