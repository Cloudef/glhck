#include "internal.h"
#include <stdio.h>  /* for printf   */
#include <stdlib.h> /* for realloc  */
#include <ctype.h>  /* for toupper  */
#include <stdarg.h> /* for va_list  */
#include <string.h> /* for strdup   */
#include <limits.h> /* for LINE_MAX */

#ifdef __APPLE__
#   include <malloc/malloc.h>
#else
#   include <malloc.h>
#endif

/* \brief output in red */
inline void _glhckRed(void)
{
   if (!_GLHCKlibrary.misc.coloredLog) return;

#if defined(__unix__) || defined(__APPLE__)
   printf("\33[31m");
#endif

#ifdef _WIN32
   HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
   SetConsoleTextAttribute(hStdout, FOREGROUND_RED
   |FOREGROUND_INTENSITY);
#endif
}

/* \brief output in green */
inline void _glhckGreen(void)
{
   if (!_GLHCKlibrary.misc.coloredLog) return;

#if defined(__unix__) || defined(__APPLE__)
   printf("\33[32m");
#endif

#ifdef _WIN32
   HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
   SetConsoleTextAttribute(hStdout, FOREGROUND_GREEN
   |FOREGROUND_INTENSITY);
#endif
}

/* \brief output in blue */
inline void _glhckBlue(void)
{
   if (!_GLHCKlibrary.misc.coloredLog) return;

#if defined(__unix__) || defined(__APPLE__)
   printf("\33[34m");
#endif

#ifdef _WIN32
   HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
   SetConsoleTextAttribute(hStdout, FOREGROUND_BLUE
   |FOREGROUND_GREEN|FOREGROUND_INTENSITY);
#endif
}

/* \brief output in yellow */
inline void _glhckYellow(void)
{
   if (!_GLHCKlibrary.misc.coloredLog) return;

#if defined(__unix__) || defined(__APPLE__)
   printf("\33[33m");
#endif

#ifdef _WIN32
   HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
   SetConsoleTextAttribute(hStdout, FOREGROUND_GREEN
   |FOREGROUND_RED|FOREGROUND_INTENSITY);
#endif
}

/* \brief output in white */
inline void _glhckWhite(void)
{
   if (!_GLHCKlibrary.misc.coloredLog) return;

#if defined(__unix__) || defined(__APPLE__)
   printf("\33[37m");
#endif

#ifdef _WIN32
   HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
   SetConsoleTextAttribute(hStdout, FOREGROUND_RED
   |FOREGROUND_GREEN|FOREGROUND_BLUE);
#endif
}

/* \brief reset output color */
inline void _glhckNormal(void)
{
   if (!_GLHCKlibrary.misc.coloredLog) return;

#if defined(__unix__) || defined(__APPLE__)
   printf("\33[0m");
#endif

#ifdef _WIN32
   HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
   SetConsoleTextAttribute(hStdout, FOREGROUND_RED
   |FOREGROUND_GREEN|FOREGROUND_BLUE);
#endif
}

/* \brief colored puts */
void _glhckPuts(const char *buffer)
{
   int i, len;

   len = strlen(buffer);
   for (i = 0; i != len; ++i) {
           if (buffer[i] == '\1') _glhckRed();
      else if (buffer[i] == '\2') _glhckGreen();
      else if (buffer[i] == '\3') _glhckBlue();
      else if (buffer[i] == '\4') _glhckYellow();
      else if (buffer[i] == '\5') _glhckWhite();
      else printf("%c", buffer[i]);
   }
   _glhckNormal();
   printf("\n");
   fflush(stdout);
}

/* \brief colored printf */
void _glhckPrintf(const char *fmt, ...)
{
   va_list args;
   char buffer[LINE_MAX];

   memset(buffer, 0, LINE_MAX);
   va_start(args, fmt);
   vsnprintf(buffer, LINE_MAX-1, fmt, args);
   va_end(args);

   _glhckPuts(buffer);
}

/* \brief split string */
size_t _glhckStrsplit(char ***dst, const char *str, const char *token)
{
   char *saveptr, *ptr, *start;
   size_t t_len, i;

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

         if (!*ptr) break;
         start=ptr;
      }
   }

   return i;
}

/* \brief free split */
void _glhckStrsplitClear(char ***dst) {
   if ((*dst)[0]) free((*dst)[0]);
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
      if (toupper(hay[i]) != toupper(needle[i])) return 1;
   return 0;
}

/* \brief strstr strings in uppercase */
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
