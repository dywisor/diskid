/*
 * diskid (main.c)
 *
 * Extended/standalone version of udev's ata_id program:
 * - accepts more than one device node
 * - accepts the --mdev(-m) option, which is a very reduced variant of
 *   --export that prints ID_BUS, ID_SERIAL and ID_WWN_WITH_EXTENSION only
 * - other ID_BUS types _may_ be added in future
 *
 * Note that --export and --mdev behave identical if diskid is built with
 * ENABLE_MINIMAL(!=0).
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
 */

/*
 * This file is based on udev's ata_id.c (src/udev/ata_id.c),
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
#include <getopt.h>
#include <string.h>
#include <libgen.h>


#include "disk_type.h"
#include "ata_id.h"
#include "util.h"

union u_specific_device_info {
   struct ata_disk_info ata;
};


static int handle_device (
   struct disk_info* const node,
   unsigned const int export, unsigned const int mdev_export,
   unsigned const int disk_type_mask, unsigned const int node_count
) {
   int retcode;
   union u_specific_device_info** buffer;
   char* varname_prefix;

   const char* const VJOIN_SEQ = "_";

   retcode        = 1;
   varname_prefix = NULL;
   buffer         = malloc ( sizeof *buffer );
   if ( buffer == NULL ) {
      goto handle_device_exit;
   }
   *buffer = NULL;

   if ( node_count > 1 ) {
      varname_prefix = join_str_double ( node->var_name, VJOIN_SEQ );
   } else {
      varname_prefix = malloc(1);
      if ( varname_prefix != NULL ) { *varname_prefix = '\0'; }
   }


   if (
      (disk_type_mask & DISK_TYPE_ATA) &&
      is_ata_disk ( node, (struct ata_disk_info** const)buffer )
   ) {
      set_disk_type_ata ( node );
   } else {
      set_disk_type_none ( node );
      fprintf ( stderr,
         "failed to detect disk type for device '%s''\n", node->device
      );
      goto handle_device_exit;
   }


   if ( (node->type == DISK_TYPE_NONE) || (*buffer == NULL) ) {
      fprintf ( stderr, "failed to get disk info!\n" );

   } else if ( export == 0 && mdev_export == 0 ) {
      if ( node->type == DISK_TYPE_ATA ) {
         set_ata_id ( node, (struct ata_disk_info* const)(*buffer) );
      }

      if ( node->disk_id != NULL ) {
         if ( node_count > 1 ) {
            printf ( "%s:%s\n", node->name, node->disk_id );
         } else {
            printf ( "%s\n", node->disk_id );
         }
         retcode = 0;
      }

   } else if ( varname_prefix != NULL ) {
      if ( node->type == DISK_TYPE_ATA ) {
         retcode = print_ata_id_vars (
            node,
            (const struct ata_disk_info* const)(*buffer),
            mdev_export, (const char* const)varname_prefix
         );
      } else {
         fprintf ( stderr, "--export is TODO!\n" );
         retcode = 2;
      }
   }

handle_device_exit:
   if ( varname_prefix != NULL ) {
      free ( varname_prefix );
      varname_prefix = NULL;
   }

   if ( buffer != NULL ) {
      if ( *buffer != NULL ) {
         free ( *buffer );
         *buffer = NULL;
      }
      free ( buffer );
   }
   return retcode;
}


int main ( const int argc, char* const* argv ) {
   int retcode            = EXIT_SUCCESS;
   struct disk_info* node = NULL;

   int i;
   unsigned int node_count;
   unsigned int exit_after_getopt;
   unsigned int want_export;
   unsigned int want_mdev_export;
   /*enum disk_type want_disk_type;*/


   static const struct option const long_options[] = {
      { "export", no_argument,       NULL, 'x' },
      { "mdev",   no_argument,       NULL, 'm' },
      { "help",   no_argument,       NULL, 'h' },
      /*{ "type",   required_argument, NULL, 't' },*/
      {0}
   };

   exit_after_getopt = 0;
   want_export       = 0;
   want_mdev_export  = 0;
   /*want_disk_type    = DISK_TYPE_ALL;*/
   while (
      ( i = getopt_long ( argc, argv, "xhm", long_options, NULL ) ) != -1
   ) {
      switch ( i ) {
         case 'h':
            fprintf ( stdout,
               (
                  /* "Usage: %s [-h] [-x] [-m] [-t <TYPE>] <DEVICE> [<DEVICE>...]\n" */
                  "Usage: %s [-h] [-x] [-m] [<DEVICE>...]\n"
                  "  -h, --help           print this help message and exit\n"
                  "  -x, --export         print environment variables\n"
                  "  -m, --mdev           print environment variables for mdev\n"
                  /*"  -t, --type <TYPE>    restrict or set disk type to TYPE\n"*/
                  "\n"
               ), basename(argv[0])
            );
            exit_after_getopt = 1;
            break;
         case 'x':
            want_export = 1;
            break;
         case 'm':
            want_mdev_export = 1;
            break;
         /* --type, -t has no functionality so far */
         /*
         case 't':
            fprintf ( stderr, "-t,--type: not implemented.\n" );
            break;
         */
         default:
            retcode = EXIT_FAILURE;
            goto main_exit;
      }
   }

   if ( exit_after_getopt == 1 ) {
      goto main_exit;

   } else if ( optind < argc ) {
      node_count = (unsigned int)(argc - optind);

      for ( i = optind; i < argc; i++ ) {
         node = init_disk_info ( argv[i] );
         if ( node == NULL ) {
            fprintf ( stderr, "failed to open device '%s'\n", argv[i] );
            retcode = EXIT_FAILURE;
            goto main_exit;
         } else if (
            handle_device (
               node, want_export, want_mdev_export, DISK_TYPE_ALL, node_count
            ) != 0
         ) {
            retcode = EXIT_FAILURE;
            goto main_exit;
         }
      }

   } else {
      fprintf ( stderr, "no device specified\n" );
      retcode = EXIT_FAILURE;
      goto main_exit;
   }



main_exit:
   fflush ( stdout );
   fflush ( stderr );

   if ( node != NULL ) {
      close_disk_info ( node );
      node = NULL;
   }

   return retcode;
}
