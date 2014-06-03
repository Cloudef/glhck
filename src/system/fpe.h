#ifndef __glhck_fpe_h__
#define __glhck_fpe_h__

#if defined(__linux__) && defined(__GNUC__)
#  define _GNU_SOURCE
#  include <fenv.h>
int feenableexcept(int excepts);
#endif

#if (defined(__APPLE__) && (defined(__i386__) || defined(__x86_64__)))
#  define OSX_SSE_FPE
#  include <xmmintrin.h>
#endif

#endif /* __glhck_fpe_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
