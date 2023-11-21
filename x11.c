#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <glad/glx.h>

static bool fullscreen = false;

Colormap colormap;
GLXContext glx_context;

void resize_x11_window(Display *display, Window window) {
  XWindowAttributes win_attrs;
  XGetWindowAttributes(display, window, &win_attrs);
  glViewport(0, 0, win_attrs.width, win_attrs.height);
}

int init_x11(Display **display, Window *window, const char* title, int window_width, int window_height) {
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
    GLX_DOUBLEBUFFER, 1,
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
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glViewport(0, 0, win_attrs.width, win_attrs.height);
  resize_x11_window(*display, *window);

  return 0;
}

void close_x11(Display **display, Window *window) {
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

void toggle_fullscreen(Display *display, Window window) {
  if (fullscreen) {
    return_fullscreen(display, window);
  } else {
    go_fullscreen(display, window);
  }

  fullscreen = !fullscreen;
}
