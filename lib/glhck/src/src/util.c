#include "internal.h"
#include <ctype.h> /* for toupper */

/* \brief split string */
int _glhckStrsplit(char ***dst, char *str, char *token)
{
   char *saveptr, *ptr, *start;
   int t_len, i;

   if (!(saveptr=strdup(str)))
      return 0;

   *dst=NULL;
   t_len=strlen(token);
   i=0;

   for (start=saveptr,ptr=start;;ptr++) {
      if (!strncmp(ptr,token,t_len) || !*ptr) {
         while (!strncmp(ptr,token,t_len)) {
            *ptr=0;
            ptr+=t_len;
         }

         if (!((*dst)=realloc(*dst,(i+2)*sizeof(char*))))
            return 0;
         (*dst)[i]=start;
         (*dst)[i+1]=NULL;
         i++;

         if (!*ptr)
            break;
         start=ptr;
      }
   }

   return i;
}

/* \brief free split */
void _glhckStrsplitClear(char ***dst) {
   if ((*dst)[0])
      free((*dst)[0]);
   free((*dst));
}

/* \brief strcmp strings in uppercase, NOTE: returns 0 on found else 1 (so you don't mess up with strcmp) */
int _glhckStrupcmp(const char *hay, const char *needle)
{
   size_t i, len;
   if ((len = strlen(hay)) != strlen(needle)) return 1;
   for (i = 0; i != len; ++i)
      if (toupper(hay[i]) != toupper(needle[i])) return 1;
   return 0;
}

/* \brief strncmp strings in uppercase, NOTE: returns 0 on found else 1 (so you don't mess up with strcmp) */
int _glhckStrnupcmp(const char *hay, const char *needle, size_t len)
{
   size_t i;
   for (i = 0; i != len; ++i)
      if (hay[i] != needle[i]) return 1;
   return 0;
}

char* _glhckStrupstr(const char *hay, const char *needle)
{
   size_t i, r, p, len, len2;
   p = 0; r = 0;
   if (!_glhckStrupcmp(hay, needle)) return (char*)hay;
   if ((len = strlen(hay)) < (len2 = strlen(needle))) return NULL;
   for (i = 0; i != len; ++i) {
      if (p == len2) return (char*)&hay[r]; /* THIS IS IT! */
      if (toupper(hay[i]) == toupper(needle[p++])) {
         if (!r) r = i; /* could this be.. */
      } else { if (r) i = r; r = 0; p = 0; } /* ..nope, damn it! */
   }
   if (p == len2) return (char*)&hay[r]; /* THIS IS IT! */
   return NULL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
