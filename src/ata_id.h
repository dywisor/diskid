/*
 * ata_id - reads product/serial number from ATA drives
 *
 * This is a standalone version of udev's (now systemd) ata_id program
 * with the following differences:
 *
 * - does not depend on udev (libudev)
 * - "-x" and "-h" options added to the help message
 * - logging removed(!): error messages are printed to stderr
 *                       and debug messages have been removed
 * - uses udev_util.h instead of libudev.h, which is a breakout of libudev
 * - ENABLE_MINIMAL affects the output of --export
 * - --mdev option
 * - unofficial, unsupported, ...
 *
 * systemd's project page is http://freedesktop.org/wiki/Software/systemd/
 * *** DO NOT REPORT BUGS THERE ***
 *
 * Copyright (C) 2013 Andre Erdmann <dywi@mailerd.de>
 *
 * diskid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 * This file is heavily based on udev's ata_id.c (src/udev/ata_id.c),
 * its license follows.
 */

/*
 * ata_id - reads product/serial number from ATA drives
 *
 * Copyright (C) 2005-2008 Kay Sievers <kay@vrfy.org>
 * Copyright (C) 2009 Lennart Poettering <lennart@poettering.net>
 * Copyright (C) 2009-2010 David Zeuthen <zeuthen@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _DISKID_ATA_ID_
#define _DISKID_ATA_ID_

#include <stdint.h>
#include <linux/hdreg.h>

#include "disk_type.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ata_disk_info {
   uint8_t     identify[512];
   uint16_t*   identify_words;
   int         is_packet_device;
   char        model[41];
   char        model_enc[256];
   char        serial[21];
   char        revision[9];
   struct hd_driveid id;
};

int is_ata_disk (
   const struct disk_info* const node,
   struct ata_disk_info** const pinfo
);

int print_ata_id_vars (
   const struct disk_info* const node,
   const struct ata_disk_info* const pinfo,
   unsigned const int mdev_export,
   const char* const prefix
);

static inline int set_ata_id (
   struct disk_info* const node, const struct ata_disk_info* const pinfo
) {
   return (pinfo == NULL) ? -1 : set_disk_id (node, pinfo->model, pinfo->serial);
}


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
