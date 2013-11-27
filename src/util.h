/*
 * util.h - small helper functions
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

#ifndef _DISKID_UTIL_
#define _DISKID_UTIL_

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

#if _GNU_SOURCE
/* use GNU strnlen */
static inline size_t util_strnlen ( const char* s, size_t maxlen ) {
   return strnlen ( s, maxlen );
}
#else
/* use naive/unoptimized strnlen() */
static inline size_t util_strnlen ( const char* s, size_t maxlen ) {
   const char* char_ptr;
   size_t len;

   char_ptr = s;
   len      = 0;

   while ( len < maxlen && *char_ptr ) {
      char_ptr++;
      len++;
   }

   return len;
}
#endif

static inline void convert_to_uppercase (
   char* const str, const size_t start, const size_t stop
) {
   size_t i;
   for ( i = start; i < stop && str[i] != '\0'; i++ ) {
      str[i] = (char)toupper ( str[i] );
   }
}

static inline char* get_uppercase ( const char* const str ) {
   char* upstr;
   upstr = strdup ( str );
   /* strlen()+1 not needed here */
   convert_to_uppercase ( upstr, 0, strlen ( str ) );
   return upstr;
}

static inline char* join_str_double (
   const char* const left, const char* const right
) {
   if ( left == NULL ) {
      if ( right == NULL ) {
         return NULL;
      } else {
         return strdup ( right );
      }
   } else if ( right == NULL ) {
      return strdup ( left );
   } else {
      size_t size_left;
      size_t size_right;
      size_t i;

      char* out_str = NULL;

      size_left  = strlen ( left );
      size_right = strlen ( right );
      out_str    = malloc ( size_left + size_right + 1 );

      if ( out_str != NULL ) {
         for ( i = 0; i < size_left; i++ ) {
            out_str[i] = left[i];
         }
         for ( i = 0; i < size_right; i++ ) {
            out_str[size_left+i] = right[i];
         }
         out_str [size_left+size_right] = '\0';
      }

      return out_str;
   }
}

static inline char* join_str_triple (
   const char* const left, const char* const middle, const char* const right
) {
   char* tmp_str = NULL;
   char* out_str = NULL;

   tmp_str = join_str_double ( left, middle );
   if ( right == NULL ) {
      out_str = tmp_str;
   } else {
      out_str = join_str_double ( tmp_str, right );
      if ( tmp_str != NULL ) {
         free ( tmp_str );
      }
   }

   return out_str;
}



#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
