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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>
#include <scsi/scsi_ioctl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <linux/hdreg.h>
#include <linux/fs.h>
#include <linux/cdrom.h>
#include <linux/bsg.h>
#include <arpa/inet.h>

#include "udev_util.h"
#include "disk_type.h"
#include "ata_id.h"

#define COMMAND_TIMEOUT_MSEC (30 * 1000)


static inline int print_mdev_ata_id_vars (
   const struct disk_info* const node,
   const struct ata_disk_info* const pinfo,
   unsigned const int mdev_export,
   const char* const prefix
);

static inline int has_wwn ( const uint8_t* const identify );
static uint64_t   get_wwn ( const uint8_t* const identify );


#if ENABLE_MINIMAL

int print_ata_id_vars (
   const struct disk_info* const node,
   const struct ata_disk_info* const pinfo,
   unsigned const int mdev_export,
   const char* const prefix
) {
   return print_mdev_ata_id_vars ( node, pinfo, mdev_export, prefix );
}

#else

static inline int print_ata_id_vars__extended (
   const struct disk_info* const node,
   const struct ata_disk_info* const pinfo,
   const char* const prefix
) {
   const struct hd_driveid* const id    = &(pinfo->id);
   const uint8_t*  const identify       = pinfo->identify;
   const uint16_t* const identify_words = pinfo->identify_words;
   uint16_t word;


   /* Set this to convey the disk speaks the ATA protocol */
   printf("%sID_ATA=1\n", prefix);

   if ((id->config >> 8) & 0x80) {
      /* This is an ATAPI device */
      switch ((id->config >> 8) & 0x1f) {
         case 0:
            printf("%sID_TYPE=cd\n", prefix);
            break;
         case 1:
            printf("%sID_TYPE=tape\n", prefix);
            break;
         case 5:
            printf("%sID_TYPE=cd\n", prefix);
            break;
         case 7:
            printf("%sID_TYPE=optical\n", prefix);
            break;
         default:
            printf("%sID_TYPE=generic\n", prefix);
            break;
      }
   } else {
      printf("%sID_TYPE=disk\n", prefix);
   }

   printf("%sID_BUS=ata\n", prefix);
   printf("%sID_MODEL=%s\n", prefix, pinfo->model);
   printf("%sID_MODEL_ENC=%s\n", prefix, pinfo->model_enc);
   printf("%sID_REVISION=%s\n", prefix, pinfo->revision);
   if (pinfo->serial[0] != '\0') {
      printf("%sID_SERIAL=%s_%s\n", prefix, pinfo->model, pinfo->serial);
      printf("%sID_SERIAL_SHORT=%s\n", prefix, pinfo->serial);
   } else {
      printf("%sID_SERIAL=%s\n", prefix, pinfo->model);
   }

   if (id->command_set_1 & (1<<5)) {
      printf ("%sID_ATA_WRITE_CACHE=1\n", prefix);
      printf ("%sID_ATA_WRITE_CACHE_ENABLED=%d\n",
         prefix, (id->cfs_enable_1 & (1<<5)) ? 1 : 0
      );
   }

   if (id->command_set_1 & (1<<10)) {
      printf("%sID_ATA_FEATURE_SET_HPA=1\n", prefix);
      printf("%sID_ATA_FEATURE_SET_HPA_ENABLED=%d\n",
         prefix, (id->cfs_enable_1 & (1<<10)) ? 1 : 0
      );

      /*
       * TODO: use the READ NATIVE MAX ADDRESS command to get the native max address
       * so it is easy to check whether the protected area is in use.
       */
   }

   if (id->command_set_1 & (1<<3)) {
      printf("%sID_ATA_FEATURE_SET_PM=1\n", prefix);
      printf("%sID_ATA_FEATURE_SET_PM_ENABLED=%d\n",
         prefix, (id->cfs_enable_1 & (1<<3)) ? 1 : 0
      );
   }

   if (id->command_set_1 & (1<<1)) {
      printf("%sID_ATA_FEATURE_SET_SECURITY=1\n", prefix);
      printf("%sID_ATA_FEATURE_SET_SECURITY_ENABLED=%d\n",
         prefix, (id->cfs_enable_1 & (1<<1)) ? 1 : 0
      );
      printf("%sID_ATA_FEATURE_SET_SECURITY_ERASE_UNIT_MIN=%d\n",
         prefix, id->trseuc * 2
      );

      if ((id->cfs_enable_1 & (1<<1))) /* enabled */ {
         if (id->dlf & (1<<8)) {
            printf("%sID_ATA_FEATURE_SET_SECURITY_LEVEL=maximum\n", prefix);
         } else {
            printf("%sID_ATA_FEATURE_SET_SECURITY_LEVEL=high\n", prefix);
         }
      }

      if (id->dlf & (1<<5)) {
         printf("%sID_ATA_FEATURE_SET_SECURITY_ENHANCED_ERASE_UNIT_MIN=%d\n",
            prefix, id->trsEuc * 2);
      }
      if (id->dlf & (1<<4)) {
         printf("%sID_ATA_FEATURE_SET_SECURITY_EXPIRE=1\n", prefix);
      }
      if (id->dlf & (1<<3)) {
         printf("%sID_ATA_FEATURE_SET_SECURITY_FROZEN=1\n", prefix);
      }
      if (id->dlf & (1<<2)) {
         printf("%sID_ATA_FEATURE_SET_SECURITY_LOCKED=1\n", prefix);
      }
   }

   if (id->command_set_1 & (1<<0)) {
      printf("%sID_ATA_FEATURE_SET_SMART=1\n", prefix);
      printf("%sID_ATA_FEATURE_SET_SMART_ENABLED=%d\n",
         prefix, (id->cfs_enable_1 & (1<<0)) ? 1 : 0
      );
   }
   if (id->command_set_2 & (1<<9)) {
      printf("%sID_ATA_FEATURE_SET_AAM=1\n", prefix);
      printf("%sID_ATA_FEATURE_SET_AAM_ENABLED=%d\n",
         prefix, (id->cfs_enable_2 & (1<<9)) ? 1 : 0
      );
      printf("%sID_ATA_FEATURE_SET_AAM_VENDOR_RECOMMENDED_VALUE=%d\n",
         prefix, id->acoustic >> 8
      );
      printf("%sID_ATA_FEATURE_SET_AAM_CURRENT_VALUE=%d\n",
         prefix, id->acoustic & 0xff
      );
   }

   if (id->command_set_2 & (1<<5)) {
      printf("%sID_ATA_FEATURE_SET_PUIS=1\n", prefix);
      printf("%sID_ATA_FEATURE_SET_PUIS_ENABLED=%d\n", prefix, (id->cfs_enable_2 & (1<<5)) ? 1 : 0);
   }

   if (id->command_set_2 & (1<<3)) {
      printf("%sID_ATA_FEATURE_SET_APM=1\n", prefix);
      printf("%sID_ATA_FEATURE_SET_APM_ENABLED=%d\n", prefix, (id->cfs_enable_2 & (1<<3)) ? 1 : 0);
      if ((id->cfs_enable_2 & (1<<3))) {
         printf("%sID_ATA_FEATURE_SET_APM_CURRENT_VALUE=%d\n", prefix, id->CurAPMvalues & 0xff);
      }
   }

   if (id->command_set_2 & (1<<0)) {
      printf("%sID_ATA_DOWNLOAD_MICROCODE=1\n", prefix);
   }

   /*
    * Word 76 indicates the capabilities of a SATA device. A PATA device shall set
    * word 76 to 0000h or FFFFh. If word 76 is set to 0000h or FFFFh, then
    * the device does not claim compliance with the Serial ATA specification and words
    * 76 through 79 are not valid and shall be ignored.
    */
   word = *((uint16_t *) identify + 76);
   if (word != 0x0000 && word != 0xffff) {
      printf("%sID_ATA_SATA=1\n", prefix);
      /*
       * If bit 2 of word 76 is set to one, then the device supports the Gen2
       * signaling rate of 3.0 Gb/s (see SATA 2.6).
       *
       * If bit 1 of word 76 is set to one, then the device supports the Gen1
       * signaling rate of 1.5 Gb/s (see SATA 2.6).
       */
      if (word & (1<<2)) {
         printf("%sID_ATA_SATA_SIGNAL_RATE_GEN2=1\n", prefix);
      }
      if (word & (1<<1)) {
         printf("%sID_ATA_SATA_SIGNAL_RATE_GEN1=1\n", prefix);
      }
   }

   /* Word 217 indicates the nominal media rotation rate of the device */
   word = *((uint16_t *) identify + 217);
   if (word != 0x0000) {
      if (word == 0x0001) {
         printf ("%sID_ATA_ROTATION_RATE_RPM=0\n", prefix); /* non-rotating e.g. SSD */
      } else if (word >= 0x0401 && word <= 0xfffe) {
         printf ("%sID_ATA_ROTATION_RATE_RPM=%d\n", prefix, word);
      }
   }

   if ( has_wwn ( pinfo->identify ) != 0 ) {
      uint64_t wwn = get_wwn ( pinfo->identify );

      printf("%sID_WWN=0x%llx\n", prefix, (unsigned long long int) wwn);
      /* ATA devices have no vendor extension */
      printf("%sID_WWN_WITH_EXTENSION=0x%llx\n", prefix, (unsigned long long int) wwn);
   }

   /* from Linux's include/linux/ata.h */
   if (identify_words[0] == 0x848a || identify_words[0] == 0x844a) {
      printf("%sID_ATA_CFA=1\n", prefix);
   } else if ((identify_words[83] & 0xc004) == 0x4004) {
      printf("%sID_ATA_CFA=1\n", prefix);
   }

   return 0;
}

int print_ata_id_vars (
   const struct disk_info* const node,
   const struct ata_disk_info* const pinfo,
   unsigned const int mdev_export,
   const char* const prefix
) {
   if ( mdev_export == 0 ) {
      return print_ata_id_vars__extended ( node, pinfo, prefix );
   } else {
      return print_mdev_ata_id_vars ( node, pinfo, mdev_export, prefix );
   }
}
#endif


static void disk_identify_get_string (
   uint8_t const identify[512],
   unsigned int offset_words,
   char* dest, size_t dest_len
) {
   unsigned int c1;
   unsigned int c2;

   while (dest_len > 0) {
      c1 = identify[offset_words * 2 + 1];
      c2 = identify[offset_words * 2];
      *dest = (char)c1;
      dest++;
      *dest = (char)c2;
      dest++;
      offset_words++;
      dest_len -= 2;
   }
}

static void disk_identify_fixup_string (
   uint8_t const identify[512], unsigned int offset_words, const size_t len
) {
   disk_identify_get_string (
      identify, offset_words,
      (char *) identify + offset_words * 2, len
   );
}

static void disk_identify_fixup_uint16 (
   uint8_t const identify[512], const unsigned int offset_words
) {
   uint16_t *p;

   p = (uint16_t *) identify;
   p[offset_words] = le16toh (p[offset_words]);
}

static int disk_identify_command (
   const int fd, void* const buf, const size_t buf_len
) {
   uint8_t cdb[12] = {
      /*
       * ATA Pass-Through 12 byte command, as described in
       *
       *  T10 04-262r8 ATA Command Pass-Through
       *
       * from http://www.t10.org/ftp/t10/document.04/04-262r8.pdf
      */
      [0] = 0xa1,     /* OPERATION CODE: 12 byte pass through */
      [1] = 4 << 1,   /* PROTOCOL: PIO Data-in */
      [2] = 0x2e,     /* OFF_LINE=0, CK_COND=1, T_DIR=1, BYT_BLOK=1, T_LENGTH=2 */
      [3] = 0,        /* FEATURES */
      [4] = 1,        /* SECTORS */
      [5] = 0,        /* LBA LOW */
      [6] = 0,        /* LBA MID */
      [7] = 0,        /* LBA HIGH */
      [8] = 0 & 0x4F, /* SELECT */
      [9] = 0xEC,     /* Command: ATA IDENTIFY DEVICE */
   };

   uint8_t sense[32] = {0};
   uint8_t *desc = sense + 8;
   struct sg_io_v4 io_v4 = {
      .guard = 'Q',
      .protocol = BSG_PROTOCOL_SCSI,
      .subprotocol = BSG_SUB_PROTOCOL_SCSI_CMD,
      .request_len = sizeof(cdb),
      .request = (uintptr_t) cdb,
      .max_response_len = sizeof(sense),
      .response = (uintptr_t) sense,
      .din_xfer_len = buf_len,
      .din_xferp = (uintptr_t) buf,
      .timeout = COMMAND_TIMEOUT_MSEC,
   };
   int ret;

   ret = ioctl(fd, SG_IO, &io_v4);
   if (ret != 0) {
      /* could be that the driver doesn't do version 4, try version 3 */
      if (errno == EINVAL) {
         struct sg_io_hdr io_hdr = {
            .interface_id = 'S',
            .cmdp = (unsigned char*) cdb,
            .cmd_len = sizeof (cdb),
            .dxferp = buf,
            .dxfer_len = buf_len,
            .sbp = sense,
            .mx_sb_len = sizeof (sense),
            .dxfer_direction = SG_DXFER_FROM_DEV,
            .timeout = COMMAND_TIMEOUT_MSEC,
         };

         ret = ioctl(fd, SG_IO, &io_hdr);
         if (ret != 0) {
            return ret;
         }
      } else {
         return ret;
      }
  }

   if (!(sense[0] == 0x72 && desc[0] == 0x9 && desc[1] == 0x0c)) {
      errno = EIO;
      return -1;
   }

   return 0;
}


static int disk_scsi_inquiry_command (
   const int fd, void* const buf, const size_t buf_len
) {
   uint8_t cdb[6] = {
      /*
      * INQUIRY, see SPC-4 section 6.4
      */
      [0] = 0x12,                /* OPERATION CODE: INQUIRY */
      [3] = (buf_len >> 8),      /* ALLOCATION LENGTH */
      [4] = (buf_len & 0xff),
   };
   uint8_t sense[32] = {0};
   struct sg_io_v4 io_v4 = {
      .guard = 'Q',
      .protocol = BSG_PROTOCOL_SCSI,
      .subprotocol = BSG_SUB_PROTOCOL_SCSI_CMD,
      .request_len = sizeof(cdb),
      .request = (uintptr_t) cdb,
      .max_response_len = sizeof(sense),
      .response = (uintptr_t) sense,
      .din_xfer_len = buf_len,
      .din_xferp = (uintptr_t) buf,
      .timeout = COMMAND_TIMEOUT_MSEC,
   };
   int ret;

   ret = ioctl(fd, SG_IO, &io_v4);
   if (ret != 0) {
      /* could be that the driver doesn't do version 4, try version 3 */
      if (errno == EINVAL) {
         struct sg_io_hdr io_hdr = {
            .interface_id = 'S',
            .cmdp = (unsigned char*) cdb,
            .cmd_len = sizeof (cdb),
            .dxferp = buf,
            .dxfer_len = buf_len,
            .sbp = sense,
            .mx_sb_len = sizeof(sense),
            .dxfer_direction = SG_DXFER_FROM_DEV,
            .timeout = COMMAND_TIMEOUT_MSEC,
         };

         ret = ioctl(fd, SG_IO, &io_hdr);
         if (ret != 0) {
            return ret;
         }

         /* even if the ioctl succeeds, we need to check the return value */
         if ( !(
               io_hdr.status        == 0 &&
               io_hdr.host_status   == 0 &&
               io_hdr.driver_status == 0
         ) ) {
            errno = EIO;
            return -1;
         }
      } else {
         return ret;
      }
   }

   /* even if the ioctl succeeds, we need to check the return value */
   if ( !(
      io_v4.device_status    == 0 &&
      io_v4.transport_status == 0 &&
      io_v4.driver_status    == 0
   ) ) {
      errno = EIO;
      return -1;
   }

   return 0;
}

static int disk_identify_packet_device_command (
   const int fd, void* const buf, const size_t buf_len
) {
   uint8_t cdb[16] = {
      /*
      * ATA Pass-Through 16 byte command, as described in
      *
      *  T10 04-262r8 ATA Command Pass-Through
      *
      * from http://www.t10.org/ftp/t10/document.04/04-262r8.pdf
      */
      [0] = 0x85,   /* OPERATION CODE: 16 byte pass through */
      [1] = 4 << 1, /* PROTOCOL: PIO Data-in */
      [2] = 0x2e,   /* OFF_LINE=0, CK_COND=1, T_DIR=1, BYT_BLOK=1, T_LENGTH=2 */
      [3] = 0,      /* FEATURES */
      [4] = 0,      /* FEATURES */
      [5] = 0,      /* SECTORS */
      [6] = 1,      /* SECTORS */
      [7] = 0,      /* LBA LOW */
      [8] = 0,      /* LBA LOW */
      [9] = 0,      /* LBA MID */
      [10] = 0,     /* LBA MID */
      [11] = 0,     /* LBA HIGH */
      [12] = 0,     /* LBA HIGH */
      [13] = 0,     /* DEVICE */
      [14] = 0xA1,  /* Command: ATA IDENTIFY PACKET DEVICE */
      [15] = 0,     /* CONTROL */
   };
   uint8_t sense[32] = {0};
   uint8_t *desc = sense + 8;
   struct sg_io_v4 io_v4 = {
      .guard = 'Q',
      .protocol = BSG_PROTOCOL_SCSI,
      .subprotocol = BSG_SUB_PROTOCOL_SCSI_CMD,
      .request_len = sizeof (cdb),
      .request = (uintptr_t) cdb,
      .max_response_len = sizeof (sense),
      .response = (uintptr_t) sense,
      .din_xfer_len = buf_len,
      .din_xferp = (uintptr_t) buf,
      .timeout = COMMAND_TIMEOUT_MSEC,
   };
   int ret;

   ret = ioctl(fd, SG_IO, &io_v4);

   if (ret != 0) {
      /* could be that the driver doesn't do version 4, try version 3 */
      if (errno == EINVAL) {
         struct sg_io_hdr io_hdr = {
            .interface_id = 'S',
            .cmdp = (unsigned char*) cdb,
            .cmd_len = sizeof (cdb),
            .dxferp = buf,
            .dxfer_len = buf_len,
            .sbp = sense,
            .mx_sb_len = sizeof (sense),
            .dxfer_direction = SG_DXFER_FROM_DEV,
            .timeout = COMMAND_TIMEOUT_MSEC,
         };

         ret = ioctl(fd, SG_IO, &io_hdr);
         if (ret != 0) {
            return ret;
         }
      } else {
         return ret;
      }
   }

   if (!(sense[0] == 0x72 && desc[0] == 0x9 && desc[1] == 0x0c)) {
          errno = EIO;
          return -1;
   }

   return 0;
}

static int disk_identify (
   const struct disk_info* const node,
   struct ata_disk_info* const pinfo
) {
   int ret;
   uint8_t inquiry_buf[36];
   int peripheral_device_type;
   int all_nul_bytes;
   int n;
   int is_packet_device = 0;

   /* init results */
   memzero(pinfo->identify, 512);

  /* If we were to use ATA PASS_THROUGH (12) on an ATAPI device
   * we could accidentally blank media. This is because MMC's BLANK
   * command has the same op-code (0x61).
   *
   * To prevent this from happening we bail out if the device
   * isn't a Direct Access Block Device, e.g. SCSI type 0x00
   * (CD/DVD devices are type 0x05). So we send a SCSI INQUIRY
   * command first... libata is handling this via its SCSI
   * emulation layer.
   *
   * This also ensures that we're actually dealing with a device
   * that understands SCSI commands.
   *
   * (Yes, it is a bit perverse that we're tunneling the ATA
   * command through SCSI and relying on the ATA driver
   * emulating SCSI well-enough...)
   *
   * (See commit 160b069c25690bfb0c785994c7c3710289179107 for
   * the original bug-fix and see http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=556635
   * for the original bug-report.)
   */
   ret = disk_scsi_inquiry_command (
      node->fd, inquiry_buf, sizeof *inquiry_buf
   );
   if (ret != 0) {
      goto out;
   }

   /* SPC-4, section 6.4.2: Standard INQUIRY data */
   peripheral_device_type = inquiry_buf[0] & 0x1f;
   if (peripheral_device_type == 0x05) {
      is_packet_device = 1;
      ret = disk_identify_packet_device_command (
         node->fd, pinfo->identify, 512
      );

   } else if (peripheral_device_type == 0x00) {
      /* OK, now issue the IDENTIFY DEVICE command */
      ret = disk_identify_command ( node->fd, pinfo->identify, 512 );
      if (ret != 0) {
         goto out;
      }
   } else {
      ret = -1;
      errno = EIO;
      goto out;
   }


   /* Check if IDENTIFY data is all NUL bytes - if so, bail */
   all_nul_bytes = 1;
   for (n = 0; n < 512; n++) {
      if (pinfo->identify[n] != '\0') {
         all_nul_bytes = 0;
         break;
      }
   }

   if (all_nul_bytes) {
      ret = -1;
      errno = EIO;
      goto out;
   }

out:
   pinfo->is_packet_device = is_packet_device;
   return ret;
}

static inline void transfer_id_data (
   const char* const str, char* const to, size_t len
) {
   util_replace_whitespace ( str, to, len );
   util_replace_chars ( to, NULL );
}


int is_ata_disk (
   const struct disk_info* const node,
   struct ata_disk_info** const pinfo
) {
   struct ata_disk_info* my_info = NULL;
   my_info  = malloc ( sizeof *my_info );
   if ( my_info == NULL ) {
      return 0;
   }
   *my_info = (struct ata_disk_info){ .is_packet_device = 0 };


   if ( disk_identify ( node, my_info ) == 0 ) {
      /*
       * fix up only the fields from the IDENTIFY data that we are going to
       * use and copy it into the hd_driveid struct for convenience
      */
      disk_identify_fixup_string(my_info->identify,  10, 20); /* serial */
      disk_identify_fixup_string(my_info->identify,  23,  8); /* fwrev */
      disk_identify_fixup_string(my_info->identify,  27, 40); /* model */
      disk_identify_fixup_uint16(my_info->identify,  0);      /* configuration */
      disk_identify_fixup_uint16(my_info->identify,  75);     /* queue depth */
      disk_identify_fixup_uint16(my_info->identify,  75);     /* SATA capabilities */
      disk_identify_fixup_uint16(my_info->identify,  82);     /* command set supported */
      disk_identify_fixup_uint16(my_info->identify,  83);     /* command set supported */
      disk_identify_fixup_uint16(my_info->identify,  84);     /* command set supported */
      disk_identify_fixup_uint16(my_info->identify,  85);     /* command set supported */
      disk_identify_fixup_uint16(my_info->identify,  86);     /* command set supported */
      disk_identify_fixup_uint16(my_info->identify,  87);     /* command set supported */
      disk_identify_fixup_uint16(my_info->identify,  89);     /* time required for SECURITY ERASE UNIT */
      disk_identify_fixup_uint16(my_info->identify,  90);     /* time required for enhanced SECURITY ERASE UNIT */
      disk_identify_fixup_uint16(my_info->identify,  91);     /* current APM values */
      disk_identify_fixup_uint16(my_info->identify,  94);     /* current AAM value */
      disk_identify_fixup_uint16(my_info->identify, 128);     /* device lock function */
      disk_identify_fixup_uint16(my_info->identify, 217);     /* nominal media rotation rate */
      memcpy(&(my_info->id), my_info->identify, sizeof my_info->id);
   }
   /* If this fails, then try HDIO_GET_IDENTITY */
   else if (ioctl(node->fd, HDIO_GET_IDENTITY, &(my_info->id)) != 0) {
      free ( my_info );
      return 0;
   }
   my_info->identify_words = (uint16_t*) my_info->identify;

   memcpy ( my_info->model, my_info->id.model, 40 );
   my_info->model[40] = '\0';
   encode_devnode_name (
      my_info->model, my_info->model_enc, sizeof my_info->model_enc
   );

   transfer_id_data (
      (const char* const)(my_info->id.model), my_info->model, 40
   );
   transfer_id_data (
      (const char* const)(my_info->id.serial_no), my_info->serial, 20
   );
   transfer_id_data (
      (const char* const)(my_info->id.fw_rev), my_info->revision, 8
   );

   *pinfo = my_info;
   return 1;
}


static inline int has_wwn ( const uint8_t* const identify ) {
   /*
    * Words 108-111 contain a mandatory World Wide Name (WWN) in the NAA IEEE Registered identifier
    * format. Word 108 bits (15:12) shall contain 5h, indicating that the naming authority is IEEE.
    * All other values are reserved.
    */
   uint16_t word;
   word = *((uint16_t *) identify + 108);
   return ((word & 0xf000) == 0x5000) ? 1 : 0;
}


static uint64_t get_wwn ( const uint8_t* const identify ) {
   uint64_t wwn;

   wwn   = *((uint16_t *) identify + 108);
   wwn <<= 16;
   wwn  |= *((uint16_t *) identify + 109);
   wwn <<= 16;
   wwn  |= *((uint16_t *) identify + 110);
   wwn <<= 16;
   wwn  |= *((uint16_t *) identify + 111);

   return wwn;
}

static inline int print_mdev_ata_id_vars (
   const struct disk_info* const node,
   const struct ata_disk_info* const pinfo,
   unsigned const int mdev_export,
   const char* const prefix
) {
   /* in --mdev(1) mode, print only
    * ID_BUS
    * ID_SERIAL
    * ID_WWN_WITH_EXTENSION
    */
   printf("%sID_BUS=ata\n", prefix);
   if (pinfo->serial[0] != '\0') {
      printf("%sID_SERIAL=%s_%s\n", prefix, pinfo->model, pinfo->serial);
   } else {
      printf("%sID_SERIAL=%s\n", prefix, pinfo->model);
   }

   if ( has_wwn ( pinfo->identify ) != 0 ) {
      uint64_t wwn = get_wwn ( pinfo->identify );

      /* ATA devices have no vendor extension */
      printf (
         "%sID_WWN_WITH_EXTENSION=0x%llx\n", prefix,
         (unsigned long long int) wwn
      );
   }
   return 0;
}
