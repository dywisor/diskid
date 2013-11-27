/***
  This provides some of systemd's/udev's functions that are required to
  realize a standalone/broken-out ata_id program.

  Except for very minor changes, copyright belongs to the authors as listed
  in the original source files, taken from systemd:

  Copyright 2008-2012 Kay Sievers
  Copyright 2010-2012 Lennart Poettering

  systemd is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  systemd is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with systemd; If not, see <http://www.gnu.org/licenses/>.


  In particular, functions from the following files (relative to systemd's
  src root as of commit f21326e604fd252600f3b99c277b30981477e4b1) have been
  taken/transferred/copied:

  - src/shared/device-nodes.c
  - src/shared/macro.h
  - src/shared/utf8.c
  - src/libudev/libudev.c

  -- Andre Erdmann <dywi@mailerd.de>,
     not claiming copyright on this file (minor changes only)
***/

#ifndef _DISKID_UDEV_UTIL
#define _DISKID_UDEV_UTIL_

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* from src/shared/macro.h */
static inline void memzero ( void* const ptr, const size_t num ) {
   memset ( ptr, 0, num );
}

int util_replace_whitespace (const char* const, char* const, size_t);
int util_replace_chars      (char* const, const char* const);
int encode_devnode_name     (const char* const, char* const, const size_t);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
