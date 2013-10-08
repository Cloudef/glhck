/* gcc -shared -Wl,-soname,libdisablexi.so disablexi.c -o libdisablexi.so
 *
 * Hacks for Open Pandora that should not be normally needed
 * Will be compiled with glhck if PANDORA option is specified when built with cmake
 * You can then copy it to your project and launch with LD_PRELOAD=/path/to/libdisablexi.so ./project
 */

#include <stdio.h>

/* Pandora crashes on mouse events caused by libxi. Workaround by overriding this... */
int XIQueryVersion(void *display, int *major, int *minor)
{
   printf("-!- [libXi HACK] Disabling Xinput[%d, %d] query for display: %p\n", (major?*major:0), (minor?*minor:0), display);
   return 1; // BadRequest
}

/* Pandora doesn't really have RandR and it causes problems... */
int XRRQueryExtension(void *display, int *eventBase, int *errorBase)
{
   printf("-!- [RandR HACK] Disabling RandR for display: %p\n", display);
   if (eventBase) *eventBase = 0;
   if (errorBase) *errorBase = 0;
   return 0; // False
}

/* Pandora doesn't really have vidmode support either... */
int XF86VidModeQueryExtension(void *display, int *eventBase, int *errorBase)
{
   printf("-!- [XF86VidMode HACK] Disabling XF86VidMode for display: %p\n", display);
   if (eventBase) *eventBase = 0;
   if (errorBase) *errorBase = 0;
   return 0; // False
}

/* vim: set ts=8 sw=3 tw=0 :*/
