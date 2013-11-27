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

/* Parts of systemd's src/shared/utf8.c file (which this file is based upon)
 * are based on the GLIB utf8 validation functions.
 * The original license text follows. */

/* gutf8.c - Operations on UTF-8 strings.
 *
 * Copyright (C) 1999 Tom Tromey
 * Copyright (C) 2000 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>

#include "udev_util.h"
#include "util.h"


static inline int char_in_range (
   const char c, const char low, const char high
) {
   return ( c >= low && c <= high ) ? 0 : 1;
}

/* from src/shared/device-nodes.c */
static inline int whitelisted_char_for_devnode (
   const char c, const char* const white
) {
   return (
      char_in_range ( c, '0', '9' ) ||
      char_in_range ( c, 'A', 'Z' ) ||
      char_in_range ( c, 'a', 'z' ) ||
      strchr("#+-.:=@_", c) ||
      ( white != NULL && strchr(white, c) != NULL )
   ) ? 1 : 0;
}

/* from src/shared/utf8.c */
static inline int is_unicode_valid(const uint32_t ch) {

        if (ch >= 0x110000) /* End of unicode space */
                return 0;
        if ((ch & 0xFFFFF800) == 0xD800) /* Reserved area for UTF-16 */
                return 0;
        if ((ch >= 0xFDD0) && (ch <= 0xFDEF)) /* Reserved */
                return 0;
        if ((ch & 0xFFFE) == 0xFFFE) /* BOM (Byte Order Mark) */
                return 0;

        return 1;
}

/* from src/shared/utf8.c */
/* count of characters used to encode one unicode char */
static int utf8_encoded_expected_len(const char* const str) {
        const unsigned char c = (unsigned char)str[0];

        if (c < 0x80)
                return 1;
        if ((c & 0xe0) == 0xc0)
                return 2;
        if ((c & 0xf0) == 0xe0)
                return 3;
        if ((c & 0xf8) == 0xf0)
                return 4;
        if ((c & 0xfc) == 0xf8)
                return 5;
        if ((c & 0xfe) == 0xfc)
                return 6;
        return 0;
}

/* from src/shared/utf8.c */
/* expected size used to encode one unicode char */
static int utf8_unichar_to_encoded_len(const int unichar) {
        if (unichar < 0x80)
                return 1;
        if (unichar < 0x800)
                return 2;
        if (unichar < 0x10000)
                return 3;
        if (unichar < 0x200000)
                return 4;
        if (unichar < 0x4000000)
                return 5;
        return 6;
}

/* from src/shared/utf8.c */
/* decode one unicode char */
static int utf8_encoded_to_unichar(const char* const str) {
        int unichar;
        int len;
        int i;

        len = utf8_encoded_expected_len(str);
        switch (len) {
        case 1:
                return (int)str[0];
        case 2:
                unichar = str[0] & 0x1f;
                break;
        case 3:
                unichar = (int)str[0] & 0x0f;
                break;
        case 4:
                unichar = (int)str[0] & 0x07;
                break;
        case 5:
                unichar = (int)str[0] & 0x03;
                break;
        case 6:
                unichar = (int)str[0] & 0x01;
                break;
        default:
                return -1;
        }

        for (i = 1; i < len; i++) {
                if (((int)str[i] & 0xc0) != 0x80)
                        return -1;
                unichar <<= 6;
                unichar |= (int)str[i] & 0x3f;
        }

        return unichar;
}

/* from src/shared/utf8.c */
/* validate one encoded unicode char and return its length */
static int utf8_encoded_valid_unichar(const char* const str) {
        int len;
        int unichar;
        int i;

        len = utf8_encoded_expected_len(str);
        if (len == 0)
                return -1;

        /* ascii is valid */
        if (len == 1)
                return 1;

        /* check if expected encoded chars are available */
        for (i = 0; i < len; i++)
                if ((str[i] & 0x80) != 0x80)
                        return -1;

        unichar = utf8_encoded_to_unichar(str);

        /* check if encoded length matches encoded value */
        if (utf8_unichar_to_encoded_len(unichar) != len)
                return -1;

        /* check if value has valid range */
        if (is_unicode_valid(unichar) == 0)
                return -1;

        return len;
}



/* from src/libudev/libudev.c */
int util_replace_whitespace (
   const char* const str, char* const to, size_t len
) {
        size_t i, j;

        /* strip trailing whitespace */
        len = util_strnlen(str, len);
        while (len && isspace(str[len-1]))
                len--;

        /* strip leading whitespace */
        i = 0;
        while (isspace(str[i]) && (i < len))
                i++;

        j = 0;
        while (i < len) {
                /* substitute multiple whitespace with a single '_' */
                if (isspace(str[i])) {
                        while (isspace(str[i]))
                                i++;
                        to[j++] = '_';
                }
                to[j++] = str[i++];
        }
        to[j] = '\0';
        return 0;
}

/* from src/libudev/libudev.c */
/* allow chars in whitelist, plain ascii, hex-escaping and valid utf8 */
int util_replace_chars ( char* const str, const char* const white ) {
        size_t i = 0;
        int replaced = 0;

        while (str[i] != '\0') {
                int len;

                if (whitelisted_char_for_devnode(str[i], white)) {
                        i++;
                        continue;
                }

                /* accept hex encoding */
                if (str[i] == '\\' && str[i+1] == 'x') {
                        i += 2;
                        continue;
                }

                /* accept valid utf8 */
                len = utf8_encoded_valid_unichar(&str[i]);
                if (len > 1) {
                        i += len;
                        continue;
                }

                /* if space is allowed, replace whitespace with ordinary space */
                if (isspace(str[i]) && white != NULL && strchr(white, ' ') != NULL) {
                        str[i] = ' ';
                        i++;
                        replaced++;
                        continue;
                }

                /* everything else is replaced with '_' */
                str[i] = '_';
                i++;
                replaced++;
        }
        return replaced;
}

/* from src/shared/device-nodes.c */
int encode_devnode_name (
   const char* const str, char* const str_enc, const size_t len
) {
        size_t i, j;

        if (str == NULL || str_enc == NULL)
                return -1;

        for (i = 0, j = 0; str[i] != '\0'; i++) {
                int seqlen;

                seqlen = utf8_encoded_valid_unichar(&str[i]);
                if (seqlen > 1) {
                        if (len-j < (size_t)seqlen)
                                goto err;
                        memcpy(&str_enc[j], &str[i], seqlen);
                        j += seqlen;
                        i += (seqlen-1);
                } else if (str[i] == '\\' || !whitelisted_char_for_devnode(str[i],NULL)) {
                        if (len-j < 4)
                                goto err;
                        sprintf(&str_enc[j], "\\x%02x", (unsigned char) str[i]);
                        j += 4;
                } else {
                        if (len-j < 1)
                                goto err;
                        str_enc[j] = str[i];
                        j++;
                }
        }
        if (len-j < 1)
                goto err;
        str_enc[j] = '\0';
        return 0;
err:
        return -1;
}

/* functions taken from other systemd/udev files */
