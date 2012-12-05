#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "tc.h"

#define ifree(x) if (x) free(x)
#define ACTC_CHECK_SYNTAX  "%d: "__STRING(func)" returned unexpected %04X\n"
#define ACTC_CALL_SYNTAX   "%d: "__STRING(func)" failed with %04X\n"

#define ACTC_CHECK(func, c)               \
{ int r;                                  \
   r = (func);                            \
   if (r != c) {                          \
      fprintf(stderr, ACTC_CHECK_SYNTAX,  \
            __LINE__, c);                 \
      goto fail;                          \
   }                                      \
}

#define ACTC_CALL(func)                   \
{ int r;                                  \
   r = (func);                            \
   if (r < 0) {                           \
      fprintf(stderr, ACTC_CALL_SYNTAX,   \
            __LINE__, -r);                \
      goto fail;                          \
   }                                      \
}


/* \brief split string */
int strsplit(char ***dst, char *str, char *token)
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

         if (!*ptr) break;
         start=ptr;
      }
   }

   return i;
}

/* \brief free split */
void strsplitClear(char ***dst) {
   if ((*dst)[0]) free((*dst)[0]);
   free((*dst));
}

int main(int argc, char **argv)
{
   ACTCData *tc = NULL;
   size_t i, num_indices, out_num_indices;
   unsigned int *in_indices = NULL, *out_indices = NULL;
   unsigned int v1, v2, v3 = 0, test;
   char buffer[LINE_MAX], **read_indices = NULL;

   if (!(fgets(buffer, LINE_MAX, stdin)))
         goto read_fail;

   num_indices = strsplit(&read_indices, buffer, " ");
   test = num_indices;
   while ((test-=3)>3);
   if (test != 3 && test != 0) goto not_valid;

   if (!(in_indices = malloc(num_indices * sizeof(unsigned int))))
      goto no_memory;
   if (!(out_indices = calloc(num_indices, sizeof(unsigned int))))
      goto no_memory;

   for (i = 0; i != num_indices; ++i)
      in_indices[i] = strtol(read_indices[i], (char **) NULL, 10);
   strsplitClear(&read_indices);
   read_indices = 0;

   if (!(tc = actcNew()))
      goto actc_fail;

   i = 0;
   ACTC_CALL(actcBeginInput(tc));
   while (i != num_indices) {
      ACTC_CALL(actcAddTriangle(tc,
               in_indices[i+0],
               in_indices[i+1],
               in_indices[i+2]));
      i+=3;
   }
   ACTC_CALL(actcEndInput(tc));
   ACTC_CALL(actcBeginOutput(tc));
   i = 0;
   while (actcStartNextPrim(tc, &v1, &v2) != ACTC_DATABASE_EMPTY) {
      if (i + (i>2?5:3) > num_indices)
         goto no_profit;
      if (i > 2) {
         out_indices[i++] = v3;
         out_indices[i++] = v1;
      }
      out_indices[i++] = v1;
      out_indices[i++] = v2;
      while (actcGetNextVert(tc, &v3) != ACTC_PRIM_COMPLETE) {
         if (i + 1 > num_indices)
            goto no_profit;
         out_indices[i++] = v3;
      }
   }
   ACTC_CALL(actcEndOutput(tc));
   out_num_indices = i;

   for (i = 0; i != out_num_indices; ++i)
      printf("%d%s",
            out_indices[i], i==out_num_indices-1?"\n":" ");

   actcDelete(tc);
   return EXIT_SUCCESS;

not_valid:
   puts("Triangles not divisiable by 3");
   goto fail;
no_memory:
   puts("No memory");
   goto fail;
read_fail:
   puts("Failed to read from stdin");
   goto fail;
actc_fail:
   puts("ACTC Init failed");
   goto fail;
no_profit:
   puts("-1");
fail:
   if (tc) actcDelete(tc);
   if (read_indices) strsplitClear(&read_indices);
   ifree(in_indices);
   ifree(out_indices);
   return EXIT_FAILURE;
}
