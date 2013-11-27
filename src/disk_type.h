/*
 * disk_type.h
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

#ifndef _DISKID_DISK_TYPE_
#define _DISKID_DISK_TYPE_

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "util.h"


#ifdef __cplusplus
extern "C" {
#endif

enum disk_type {
   DISK_TYPE_NONE = 0,
   DISK_TYPE_ATA  = 1,
   DISK_TYPE_SCSI = 2,
   DISK_TYPE_ALL  = (1<<2) - 1,
};

struct disk_info {
   const char*    device;
   const char*    name;
   char*          var_name;
   enum disk_type type;
   int            fd;
   char*          disk_id;
};


static inline void set_disk_type_none ( struct disk_info* const pnode ) {
   pnode->type = DISK_TYPE_NONE;
}

static inline void set_disk_type_ata ( struct disk_info* const pnode ) {
   pnode->type = DISK_TYPE_ATA;
}

static inline void set_disk_type_scsi ( struct disk_info* const pnode ) {
   pnode->type = DISK_TYPE_SCSI;
}


static inline struct disk_info* init_disk_info ( const char* const device ) {
   int fd;
   struct disk_info* pnode = NULL;

   if ( device != NULL ) {
      /* for meaningful return values, device should not be NULL */
      fd = open ( device, O_RDONLY|O_NONBLOCK );
      if ( fd >= 0 ) {
         pnode = malloc ( sizeof *pnode );
         if ( pnode != NULL ) {
            *pnode = (struct disk_info) {
               .device = device,
               .name = basename ( (char*)device ),
               .var_name = NULL,
               .type = DISK_TYPE_NONE,
               .fd = fd,
               .disk_id = NULL,
            };
            pnode->var_name = get_uppercase ( pnode->name );
         }
      }
   }

   return pnode;
}

static inline void close_disk_info ( struct disk_info* pnode ) {
   if ( pnode->fd >= 0 ) {
      close ( pnode->fd );
      pnode->fd = -1;
   }
   if ( pnode->disk_id != NULL ) {
      free ( pnode->disk_id );
      pnode->disk_id = NULL;
   }
   if ( pnode->var_name != NULL ) {
      free ( pnode->var_name );
   }
   free(pnode);
}

int set_disk_id (
   struct disk_info* const node,
   const char* const model, const char* const serial
);



#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
