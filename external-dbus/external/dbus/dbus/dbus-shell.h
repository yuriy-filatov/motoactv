/* -*- mode: C; c-file-style: "gnu" -*- */
/* dbus-shell.h Shell command line utility functions.
 *
 * Copyright (C) 2002, 2003  Red Hat, Inc.
 * Copyright (C) 2003 CodeFactory AB
 *
 * Licensed under the Academic Free License version 2.1
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


#ifndef DBUS_SHELL_H
#define DBUS_SHELL_H

DBUS_BEGIN_DECLS

char*       _dbus_shell_quote      (const char *unquoted_string);
char*       _dbus_shell_unquote    (const char *quoted_string);
dbus_bool_t _dbus_shell_parse_argv (const char *command_line,
                                    int        *argcp,
                                    char       ***argvp,
				    DBusError  *error);

DBUS_END_DECLS

#endif /* DBUS_SHELL_H */


