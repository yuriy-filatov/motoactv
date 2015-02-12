/*
 * strlen.c: return string length
 *
 * Copyright (C) 2001  Erik Mouw (J.A.K.Mouw@its.tudelft.nl)
 *
 * Copyright 2006 Motorola, Inc.
 *
 * $Id: strlen.c,v 1.1 2001/10/07 22:58:56 erikm Exp $
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
 * 04-Oct-2006  Motorola        Removed BLOB includes.
 */

#ident "$Id: strlen.c,v 1.1 2001/10/07 22:58:56 erikm Exp $"


int strlen(const char *s)
{
	int i = 0;

	for(;*s != '\0'; s++)
		i++;
	
	return i;
}

