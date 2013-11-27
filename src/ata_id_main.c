/*
 * ata_id_main.c
 *
 * Standalone version of udev's ata_id program.
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

static int handle_device (
   struct disk_info* const node, unsigned const int export
) {
   const char* const EMPTY_STR = "";

   int retcode;
   struct ata_disk_info** ppinfo;

   retcode  = 1;
   ppinfo   = malloc ( sizeof *ppinfo );
   if ( ppinfo == NULL ) { return 2; }
   *ppinfo  = NULL;


   if ( is_ata_disk ( node, ppinfo ) == 0 ) {
      fprintf ( stderr, "'%s' is not an ATA device\n", node->device );

   } else {
      set_disk_type_ata ( node );

      if ( (node->type != DISK_TYPE_ATA) || (*ppinfo == NULL) ) {
         fprintf ( stderr, "failed to get disk info!\n" );

      } else if ( export == 0 ) {
         set_ata_id ( node, *ppinfo );
         if ( node->disk_id != NULL ) {
            printf ( "%s\n", node->disk_id );
            retcode = 0;
         }
      } else {
         retcode = print_ata_id_vars ( node, *ppinfo, 0, EMPTY_STR );
      }
   }


   if ( ppinfo != NULL ) {
      if ( *ppinfo != NULL ) {
         free ( *ppinfo );
         *ppinfo = NULL;
      }
      free ( ppinfo );
   }
   return retcode;
}


int main ( const int argc, char* const* argv ) {
   int retcode            = EXIT_SUCCESS;
   struct disk_info* node = NULL;

   int i;
   unsigned int exit_after_getopt;
   unsigned int want_export;


   static const struct option const long_options[] = {
      { "export", no_argument,       NULL, 'x' },
      { "help",   no_argument,       NULL, 'h' },
      {0}
   };

   exit_after_getopt = 0;
   want_export       = 0;
   while (
      ( i = getopt_long ( argc, argv, "xh", long_options, NULL ) ) != -1
   ) {
      switch ( i ) {
         case 'h':
            fprintf ( stdout,
               (
                  "Usage: %s [-h] [-x] <DEVICE>\n"
                  "  -h, --help           print this help message and exit\n"
                  "  -x, --export         print environment variables\n"
                  "\n"
               ), basename(argv[0])
            );
            exit_after_getopt = 1;
            break;
         case 'x':
            want_export = 1;
            break;
         default:
            retcode = EXIT_FAILURE;
            exit_after_getopt = 1;
      }
   }

   if ( exit_after_getopt == 0 ) {
      if ( optind < argc ) {
         node = init_disk_info ( argv[optind] );
         if ( node == NULL ) {
            fprintf ( stderr, "failed to open device '%s'\n", argv[i] );
            retcode = EXIT_FAILURE;
         } else if ( handle_device ( node, want_export ) != 0 ) {
            retcode = EXIT_FAILURE;
         }
      } else {
         fprintf ( stderr, "no device specified\n" );
         retcode = EXIT_FAILURE;
      }
   }


   fflush ( stdout );
   fflush ( stderr );

   if ( node != NULL ) {
      close_disk_info ( node );
      node = NULL;
   }

   return retcode;
}
