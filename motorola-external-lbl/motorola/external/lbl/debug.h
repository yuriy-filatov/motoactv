/*
 * debug.h: Include file for Linux Boot Loader debugging output driver.
 * 
 * Copyright 2006 Motorola, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* Date         Author          Comment
 * ===========  ==============  ==============================================
 * 04-Oct-2006  Motorola        Initial revision.
 */

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "mach-zas/mach-debug.h"

#ifndef STDOUT_DEVICE
/*
 * If mach-debug.h didn't properly set STDOUT_DEVICE, set it to something
 * unreasonable.
 */
#define STDOUT_DEVICE   0xFFFFFFFF
#endif

#endif /* __DEBUG_H__ */
