/*
 * disk_type.c
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

#include <stdlib.h>
#include <string.h>

#include "disk_type.h"
#include "util.h"


int set_disk_id (
   struct disk_info* const node,
   const char* const model, const char* const serial
) {
   if ( node->disk_id != NULL ) {
      free ( node->disk_id );
      node->disk_id = NULL;
   }

   if ( model == NULL || model[0] == '\0' ) {
      return 1;

   } else if ( serial == NULL || serial[0] == '\0' ) {
      /* copy model to node->disk_id */
      /* node->disk_id = model; */ /* ref */
      node->disk_id = strdup ( model );
   } else {
      node->disk_id = join_str_triple ( model, "_", serial );
   }

   return ( node->disk_id != NULL ) ? 0 : 2;
}
