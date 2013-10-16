/* gcc -std=c99 -shared -Wl,-soname,libpandorahck.so pandorahck.c -o libpandorahck.so
 *
 * Hacks for Open Pandora that should not be normally needed
 * Will be compiled with glhck if PANDORA option is specified when built with cmake
 * You can then copy it to your project and launch with LD_PRELOAD=/path/to/libpandorahck.so ./project
 *
 * Hacks:
 * 1. Report that pandora doesn't support libxi to avoid crash on any mouse event.
 * 2. Grab all input to window to avoid popular Openbox PND keybind conflicts.
 *    (can be disabled with PANDORAHCK_NO_GRAB)
 * 3. Force framebuffer EGL context to increase FPS. Assumes GLFW, but might work otherwere as well.
 *    (can be disabled with PANDORAHCK_NO_FRAMEBUFFER)
 */

#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>

/* X11 specific Pandora hacks */
#include <X11/Xlib.h>

/* Pandora crashes on mouse events caused by libxi. Workaround by overriding this... */
Status XIQueryVersion(Display *display, int *major, int *minor)
{
   printf("-!- [X11 HACK] Disabling Xinput[%d, %d] query for display: %p\n", (major?*major:0), (minor?*minor:0), display);
   return BadRequest;
}

#if !PANDORAHCK_NO_GRAB
/* Grab all input to not allow custom WM key combos like the openbox setup on Pandora. */
int XMapRaised(Display *display, Window w)
{
   static int (*hooked)(Display*, Window) = NULL;
   if (!hooked) hooked = dlsym(RTLD_NEXT, __FUNCTION__);
   int ret = hooked(display, w);
   for (int i = 0; i < 1000; i++) {
      if (!XGrabKeyboard(display, w, True, GrabModeAsync, GrabModeAsync, CurrentTime)) {
         printf("-!- [X11 HACK] Grabbed input for window: %lu\n", w);
         return ret;
      }
      usleep(1000);
   }
   printf("-!- [X11 HACK] Could not grab keyboard for displau and window: %p, %lu\n", display, w);
   return ret;
}
#endif /* PANDORAHCK_NO_GRAB */

#if !PANDORAHCK_NO_FRAMEBUFFER
/* Lets create Framebuffer EGL/OpenGL context for Pandora instead.
 * We will use fullscreen X11 window however to catch input and avoid display fighting with framebuffer. */
#include <EGL/egl.h>

/* We keep count of how many configurations GLFW has checked out. */
static EGLint _hackEGLConfigurations = 0;

/* When getting EGL display we don't want to pass native display as argument. */
EGLDisplay eglGetDisplay(NativeDisplayType native_display)
{
   (void)native_display;
   static EGLDisplay (*hooked)(NativeDisplayType) = NULL;
   if (!hooked) hooked = dlsym(RTLD_NEXT, __FUNCTION__);
   printf("-!- [EGL HACK] Intercepting eglGetDisplay with argument EGL_DEFAULT_DISPLAY\n");
   return hooked(EGL_DEFAULT_DISPLAY);
}

/* When creating EGL window surface, we don't want to pass our X11 window. */
EGLSurface eglCreateWindowSurface(EGLDisplay display, EGLConfig config, NativeWindowType native_window, const EGLint *attrib_list)
{
   (void)native_window;
   static EGLSurface (*hooked)(EGLDisplay, EGLConfig, NativeWindowType, const EGLint *attrib_list) = NULL;
   if (!hooked) hooked = dlsym(RTLD_NEXT, __FUNCTION__);
   printf("-!- [EGL HACK] Intercepting eglCreateWindowSurface to ignore native_window argument\n");
   printf("-!- [EGL HACK] We should now have framebuffer context \\o/\n");
   return hooked(display, config, 0, attrib_list);
}

/* Intercept number of configuration we get from EGL and use this information in eglGetConfigAttrib */
EGLBoolean eglGetConfigs(EGLDisplay display, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
   static EGLBoolean (*hooked)(EGLDisplay, EGLConfig*, EGLint, EGLint*) = NULL;
   if (!hooked) hooked = dlsym(RTLD_NEXT, __FUNCTION__);
   EGLBoolean ret = hooked(display, configs, config_size, num_config);
   _hackEGLConfigurations = *num_config;
   printf("-!- [EGL HACK] Intercepting eglGetConfigs (configurations: %u)\n", *num_config);
   return ret;
}

/* When GLFW is compiled with _GLFW_X11 it expects that EGL_NATIVE_VISUAL_ID is not 0 in valid configurations.
 * We must return 0 after glfw has cycled all possible configurations in "chooseFBConfigs" to make things work.
 * Thus we intercept eglGetConfigs and count until we start returning real results.
 *
 * This is quite dirty hack, but I'm not sure if providing these kinds of X11/Framebuffer mix contexts are GLFW's aim. */
EGLBoolean eglGetConfigAttrib(EGLDisplay display, EGLConfig config, EGLint attribute, EGLint *value)
{
   static EGLBoolean (*hooked)(EGLDisplay, EGLConfig, EGLint, EGLint*) = NULL;
   if (!hooked) hooked = dlsym(RTLD_NEXT, __FUNCTION__);

   if (_hackEGLConfigurations) {
      switch (attribute) {
         case EGL_NATIVE_VISUAL_ID:
            *value = 1;
            printf("-!- [EGL HACK] Intercepting eglGetConfigAttrib to return something on EGL_NATIVE_VISUAL_ID\n");
            --_hackEGLConfigurations;
            return EGL_TRUE;
         default:break;
      }
   }

   return hooked(display, config, attribute, value);
}
#endif /* PANDORAHCK_NO_FRAMEBUFFER */

/* vim: set ts=8 sw=3 tw=0 :*/
