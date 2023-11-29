#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <X11/Xatom.h>
#include <glad/glx.h>

Colormap colormap;
GLXContext glx_context;

void x11_resize_window(Display *display, Window window) {
  XWindowAttributes win_attrs;
  XGetWindowAttributes(display, window, &win_attrs);
  glViewport(0, 0, win_attrs.width, win_attrs.height);
}

int x11_init(Display **display, Window *window, const char* title, int window_width, int window_height) {
  *display = XOpenDisplay(NULL);

  if (display == NULL) {
    fprintf(stderr, "Cannot open display\n");
    return 1;
  }

  int screen = DefaultScreen(*display);
  Window root = RootWindow(*display, screen);
  Visual *visual = DefaultVisual(*display, screen);

  colormap = XCreateColormap(*display, root, visual, AllocNone);

  XSetWindowAttributes attributes;
  attributes.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
    StructureNotifyMask | ButtonPressMask;
  attributes.colormap = colormap;

  *window = XCreateWindow(*display, root, 0, 0, window_width, window_height, 0,
      DefaultDepth(*display, screen), InputOutput, visual,
      CWColormap | CWEventMask, &attributes);

  // hints to the WM that the window is a dialog window which makes it a floating
  // window in titling WMs (only tested on DWM though)
  Atom net_wm_window_type = XInternAtom(*display, "_NET_WM_WINDOW_TYPE", False);
  Atom net_wm_window_type_dialog = XInternAtom(*display, "_NET_WM_WINDOW_TYPE_DIALOG", False);
  XChangeProperty(*display, *window, net_wm_window_type, XA_ATOM, 32, PropModeReplace, (unsigned char*)&net_wm_window_type_dialog, 1);

  XMapWindow(*display, *window);
  XStoreName(*display, *window, title);

  if (!window) {
    printf("Failed to create window\n");
    return 1;
  }

  int glx_version = gladLoaderLoadGLX(*display, screen);

  if (!glx_version) {
    printf("Failed to load GLX\n");
    return 1;
  }
  printf("Loaded GLX %d.%d\n",
      GLAD_VERSION_MAJOR(glx_version), GLAD_VERSION_MINOR(glx_version));

  GLint visual_attributes[] = {
    GLX_RENDER_TYPE, GLX_RGBA_BIT,
    GLX_DEPTH_SIZE, 24,
    GLX_DOUBLEBUFFER, 1,
    GLX_SAMPLES, 4, // MSAA
    None
  };

  int num_fbc = 0;
  GLXFBConfig *fbc =
    glXChooseFBConfig(*display, screen, visual_attributes, &num_fbc);

  GLint context_attributes[] = {
    GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
    GLX_CONTEXT_MINOR_VERSION_ARB, 3,
    GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
    None
  };

  glx_context = glXCreateContextAttribsARB(*display, fbc[0], NULL, 1,
      context_attributes);

  if (!glx_context) {
    printf("Failed to create GLX context\n");
    return 1;
  }

  glXMakeCurrent(*display, *window, glx_context);

  int gl_version = gladLoaderLoadGL();

  if (!gl_version) {
    printf("Failed to load GL\n");
    return 1;
  }
  printf("Loaded GL %d.%d\n",
      GLAD_VERSION_MAJOR(gl_version), GLAD_VERSION_MINOR(gl_version));

  XWindowAttributes win_attrs;
  XGetWindowAttributes(*display, *window, &win_attrs);

  glad_glXSwapIntervalEXT(*display, *window, 1);
  // we enable blending for text
  glEnable(GL_BLEND);
  glEnable(GL_MULTISAMPLE);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glViewport(0, 0, win_attrs.width, win_attrs.height);
  x11_resize_window(*display, *window);

  XFree(fbc);

  return 0;
}

void x11_close(Display **display, Window *window) {
  glXMakeCurrent(*display, 0, 0);
  glXDestroyContext(*display, glx_context);

  XDestroyWindow(*display, *window);
  XFreeColormap(*display, colormap);
  XCloseDisplay(*display);

  gladLoaderUnloadGLX();
}

void go_fullscreen(Display *display, Window window)
{
  XEvent xev;
  Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);
  Atom fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
  memset(&xev, 0, sizeof(xev));
  xev.type = ClientMessage;
  xev.xclient.window = window;
  xev.xclient.message_type = wm_state;
  xev.xclient.format = 32;
  xev.xclient.data.l[0] = 1; // _NET_WM_STATE_ADD
  xev.xclient.data.l[1] = fullscreen;
  xev.xclient.data.l[2] = 0;
  XSendEvent(display, DefaultRootWindow(display), False,
    SubstructureNotifyMask | SubstructureRedirectMask, &xev);
}

void return_fullscreen(Display *display, Window window)
{
  XEvent xev;
  Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);
  Atom fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
  memset(&xev, 0, sizeof(xev));
  xev.type = ClientMessage;
  xev.xclient.window = window;
  xev.xclient.message_type = wm_state;
  xev.xclient.format = 32;
  xev.xclient.data.l[0] = 0; // _NET_WM_STATE_REMOVE
  xev.xclient.data.l[1] = fullscreen;
  xev.xclient.data.l[2] = 0;
  XSendEvent(display, DefaultRootWindow(display), False,
    SubstructureNotifyMask | SubstructureRedirectMask, &xev);
}

void x11_toggle_fullscreen(bool fullscreen, Display *display, Window window) {
  if (fullscreen) {
    return_fullscreen(display, window);
  } else {
    go_fullscreen(display, window);
  }
}

void x11_get_screen_size(Display *display, int *width, int *height) {
  Screen *screen = DefaultScreenOfDisplay(display);

  *width = screen->width;
  *height = screen->height;
}

void x11_make_window_non_resizable(Display *display, Window window, int width, int height) {
  XSizeHints *size_hints = XAllocSizeHints();
  size_hints->flags = PMinSize | PMaxSize;
  size_hints->min_width = width;
  size_hints->min_height = height;
  size_hints->max_width = width;
  size_hints->max_height = height;
  XSetWMNormalHints(display, window, size_hints);
  XFree(size_hints);
}
