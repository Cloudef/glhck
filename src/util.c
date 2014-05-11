#include "internal.h"
#include <stdlib.h> /* for malloc   */
#include <stdio.h>  /* for printf   */
#include <ctype.h>  /* for toupper  */
#include <stdarg.h> /* for va_list  */

#ifdef __WIN32__
#  include <windows.h>
#endif

/* \brief output in red */
inline void _glhckRed(void)
{
   if (!GLHCKM()->coloredLog) return;

#if defined(__unix__) || defined(__APPLE__)
   printf("\33[31m");
#endif

#ifdef _WIN32
   HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
   SetConsoleTextAttribute(hStdout, FOREGROUND_RED|FOREGROUND_INTENSITY);
#endif
}

/* \brief output in green */
inline void _glhckGreen(void)
{
   if (!GLHCKM()->coloredLog) return;

#if defined(__unix__) || defined(__APPLE__)
   printf("\33[32m");
#endif

#ifdef _WIN32
   HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
   SetConsoleTextAttribute(hStdout, FOREGROUND_GREEN|FOREGROUND_INTENSITY);
#endif
}

/* \brief output in blue */
inline void _glhckBlue(void)
{
   if (!GLHCKM()->coloredLog) return;

#if defined(__unix__) || defined(__APPLE__)
   printf("\33[34m");
#endif

#ifdef _WIN32
   HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
   SetConsoleTextAttribute(hStdout, FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_INTENSITY);
#endif
}

/* \brief output in yellow */
inline void _glhckYellow(void)
{
   if (!GLHCKM()->coloredLog) return;

#if defined(__unix__) || defined(__APPLE__)
   printf("\33[33m");
#endif

#ifdef _WIN32
   HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
   SetConsoleTextAttribute(hStdout, FOREGROUND_GREEN|FOREGROUND_RED|FOREGROUND_INTENSITY);
#endif
}

/* \brief output in white */
inline void _glhckWhite(void)
{
   if (!GLHCKM()->coloredLog) return;

#if defined(__unix__) || defined(__APPLE__)
   printf("\33[37m");
#endif

#ifdef _WIN32
   HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
   SetConsoleTextAttribute(hStdout, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE);
#endif
}

/* \brief reset output color */
inline void _glhckNormal(void)
{
   if (!GLHCKM()->coloredLog) return;

#if defined(__unix__) || defined(__APPLE__)
   printf("\33[0m");
#endif

#ifdef _WIN32
   HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
   SetConsoleTextAttribute(hStdout, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE);
#endif
}

/* \brief colored puts */
void _glhckPuts(const char *buffer)
{
   size_t len = strlen(buffer);
   for (size_t i = 0; i != len; ++i) {
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
   char buffer[2048];
   memset(buffer, 0, sizeof(buffer));

   va_list args;
   va_start(args, fmt);
   vsnprintf(buffer, sizeof(buffer)-1, fmt, args);
   va_end(args);

   _glhckPuts(buffer);
}

/* \brief strdup without the tracking */
char* _glhckStrdupNoTrack(const char *s)
{
   size_t size;
   if (!s || !(size = strlen(s) + 1))
      return NULL;

   char *s2;
   if (!(s2 = calloc(1, size)))
      return NULL;

   memcpy(s2, s, size-1);
   return s2;
}

/* \brief split string */
size_t _glhckStrsplit(char ***dst, const char *str, const char *token)
{
   char *saveptr;
   if (!(saveptr = _glhckStrdupNoTrack(str)))
      return 0;

   size_t i, t_len = strlen(token);
   char *start = saveptr, *ptr = saveptr;
   for (i = 0, *dst = NULL ;; ++ptr) {
      if (*ptr && strncmp(ptr, token, t_len)) continue;

      while (!strncmp(ptr, token, t_len)) {
         *ptr = 0;
         ptr += t_len;
      }

      if (!((*dst) = realloc(*dst, (i + 2) * sizeof(char*)))) {
         free(saveptr);
         return 0;
      }

      (*dst)[i++] = start;
      (*dst)[i] = NULL;
      if (!*ptr) break;
      start = ptr;
   }

   return i;
}

/* \brief free split */
void _glhckStrsplitClear(char ***dst) {
   if ((*dst)[0]) free((*dst)[0]);
   free((*dst));
}

int _glhckStrupcmp(const char *hay, const char *needle)
{
   size_t len, len2;
   if ((len = strlen(hay)) != (len2 = strlen(needle)))
      return hay[len] - needle[len2];

   return _glhckStrnupcmp(hay, needle, len);
}

int _glhckStrnupcmp(const char *hay, const char *needle, size_t len)
{
   const unsigned char *p1 = (const unsigned char*)hay;
   const unsigned char *p2 = (const unsigned char*)needle;

   unsigned char a = 0, b = 0;
   for (size_t i = 0; len > 0; --len, ++i) {
      if ((a = toupper(*p1++)) != (b = toupper(*p2++)))
         return a - b;
   }

   return a - b;
}

char* _glhckStrupstr(const char *hay, const char *needle)
{

   size_t len, len2;
   if ((len = strlen(hay)) < (len2 = strlen(needle)))
      return NULL;

   if (!_glhckStrnupcmp(hay, needle, len2))
      return (char*)hay;

   size_t r = 0, p = 0;
   for (size_t i = 0; i < len; ++i) {
      if (p == len2)
         return (char*)hay + r;

      if (toupper(hay[i]) == toupper(needle[p++])) {
         if (!r)
            r = i;
      } else {
         if (r)
            i = r;
         r = p = 0;
      }
   }

   return (p == len2 ? (char*)hay + r : NULL);
}

/* vim: set ts=8 sw=3 tw=0 :*/
