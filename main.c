#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <glad/glx.h>
#include "3rdparty/stb/stb_image.h"

#include "x11.h"
#include "cudoku.h"

// TODO: everything from scratch
// X11 for windowing and input
// OpenGL for graphics
// OpenAL for audio ?? (copilot suggested this. I do want to do some audio desu)
// Simply port the js algo for sudoku generation
// Maybe after the game works fine we can use some winapi to add windows support

void usage() {
  printf("Usage: cudoku [OPTION]\n\n");
  printf("  %-20s%-20s", "-h, --help", "prints this help message\n");
  printf("  %-20s%-20s", "--use-texture", "use a texture instead of generating the grid using shaders\n");
}

Display *display;
Window window;

int main(int argc, char *argv[]) {
  bool use_texture = false;

  if (argc > 1) {
    char *flag = argv[1];
    if (strcmp(flag, "--use-texture") == 0) {
      use_texture = true;
    }
    if (strcmp(flag, "-h") == 0 || strcmp(flag, "--help") == 0) {
      usage();
      return 0;
    }
  }

  int res = init_x11(&display, &window);
  if (res < 0) return 1;

  // 900 is the initial width and height
  int width = 900, height = 900;
  // scale factors for the entire board to nicely fit as a square inside
  // the window
  float x_scale = 1.f, y_scale = 1.f;

  Shader grid_shader;
  if (!use_texture)
    grid_shader = create_shader("shaders/board_v.glsl", "shaders/grid_f.glsl");
  else
    grid_shader = create_shader("shaders/board_v.glsl", "shaders/board_f.glsl");
 
  int vao = prepare_bg(use_texture);

  bool quit = false;
  while (!quit) {
    while (XPending(display)) {
      XEvent xev;
      XNextEvent(display, &xev);

      if (xev.type == ConfigureNotify) {
        XConfigureEvent xce = xev.xconfigure;

        if (xce.width != width || xce.height != height) {
          width = xce.width;
          height = xce.height;
          resize_x11_window(display, window);
          set_scale_factor(xce.width, xce.height, &x_scale, &y_scale);
        }

      } else if (xev.type == KeyPress) {
        if (XLookupKeysym(&xev.xkey, 0) == XK_Escape || XLookupKeysym(&xev.xkey, 0) == XK_q) {
          quit = true;
          break;
        }
      }
    }

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    float transform[4][4] = {
      {x_scale, 0, 0, 0},
      {0, y_scale, 0, 0},
      {0, 0, 1, 0},
      {0, 0, 0, 1},
    };

    if (!use_texture)
      draw_bg_grid_shader(grid_shader, vao, (float *)transform, width < height ? width : height);
    else
      draw_bg_grid_texture(grid_shader, vao, (float *)transform);

    glXSwapBuffers(display, window);
  }

  close_x11(&display, &window);

  return 0;
}

