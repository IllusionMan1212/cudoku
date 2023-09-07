#include <stdbool.h>

#include <glad/glx.h>

#include "x11.h"
#include "cudoku.h"

// TODO: everything from scratch
// X11 for windowing and input
// OpenGL for graphics
// OpenAL for audio ?? (copilot suggested this. I do want to do some audio desu)
// Simply port the js algo for sudoku generation
// Maybe after the game works fine we can use some winapi to add windows support

void scale_bg_rect(int width, int height, float *x, float *y) {
  if (width > height) {
    float ratio = (float)height / (float)width;
    *x = ratio;
    *y = 1.f;
  } else {
    float ratio = (float)width / (float)height;
    *x = 1.f;
    *y = ratio;
  }
}

Display *display;
Window window;

int main() {
  int res = init_x11(&display, &window);
  if (res < 0) return 1;

  // 900 is the initial width and height
  int width = 900, height = 900;
  // scale factors for the entire board to nicely fit as a square inside
  // the window
  float x_scale = 1.f, y_scale = 1.f;

  Shader board_shader = create_shader("shaders/board_v.glsl", "shaders/board_f.glsl");
  Shader grid_shader = create_shader("shaders/grid_v.glsl", "shaders/grid_f.glsl");
  unsigned int board_vao = prepare_bg_rect(board_shader);

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
          scale_bg_rect(xce.width, xce.height, &x_scale, &y_scale);
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

    draw_bg_rect(board_shader, board_vao, x_scale, y_scale);
    draw_bg_lines(grid_shader, x_scale, y_scale, width < height ? width : height);

    glXSwapBuffers(display, window);
  }

  close_x11(&display, &window);

  return 0;
}

