#include <stdio.h>

#include <glad/glx.h>

static const uint win_width = 900;
static const uint win_height = 900;

Colormap colormap;
GLXContext context;

void resize_x11_window(Display *display, Window window) {
  XWindowAttributes win_attrs;
  XGetWindowAttributes(display, window, &win_attrs);
  glViewport(0, 0, win_attrs.width, win_attrs.height);
}

int init_x11(Display **display, Window *window) {
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

  *window = XCreateWindow(*display, root, 0, 0, win_width, win_height, 0,
      DefaultDepth(*display, screen), InputOutput, visual,
      CWColormap | CWEventMask, &attributes);

  XMapWindow(*display, *window);
  XStoreName(*display, *window, "Cudoku");

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

  context = glXCreateContextAttribsARB(*display, fbc[0], NULL, 1,
      context_attributes);

  if (!context) {
    printf("Failed to create GLX context\n");
    return 1;
  }

  glXMakeCurrent(*display, *window, context);

  int gl_version = gladLoaderLoadGL();

  if (!gl_version) {
    printf("Failed to load GL\n");
    return 1;
  }
  printf("Loaded GL %d.%d\n",
      GLAD_VERSION_MAJOR(gl_version), GLAD_VERSION_MINOR(gl_version));

  XWindowAttributes win_attrs;
  XGetWindowAttributes(*display, *window, &win_attrs);

  // we enable blending for text
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glViewport(0, 0, win_attrs.width, win_attrs.height);
  resize_x11_window(*display, *window);

  return 0;
}

void close_x11(Display **display, Window *window) {
  glXMakeCurrent(*display, 0, 0);
  glXDestroyContext(*display, context);

  XDestroyWindow(*display, *window);
  XFreeColormap(*display, colormap);
  XCloseDisplay(*display);

  gladLoaderUnloadGLX();
}

