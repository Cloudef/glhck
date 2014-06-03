#ifndef _GLHCK_TLS

#if defined(_MSC_VER)
#  define _GLHCK_TLS __declspec(thread)
#  define _GLHCK_TLS_FOUND
#elif defined(__GNUC__)
#  define _GLHCK_TLS __thread
#  define _GLHCK_TLS_FOUND
#else
#  define _GLHCK_TLS
#  warning "No Thread-local storage! Multi-context glhck applications may have unexpected behaviour!"
#endif

#endif /* _GLHCK_TLS */

/* vim: set ts=8 sw=3 tw=0 :*/
