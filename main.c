#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <glad/glx.h>
#include "3rdparty/stb/stb_image.h"

#include "x11.h"
#include "cudoku.h"
#include "font.h"

void usage() {
  printf("Usage: cudoku [OPTION]\n\n");
  printf("  %-20s%-20s", "-h, --help", "prints this help message\n");
  printf("  %-20s%-20s", "--use-texture", "use a texture instead of generating the grid using shaders\n");
  printf("  %-20s%-20s", "-f, --font FONT", "use a custom font file to render text\n");
}

Display *display;
Window window;

int main(int argc, char *argv[]) {
  const char *font_path = "assets/fonts/Rubik/Rubik-VariableFont_wght.ttf";
  /* const char *font_path = "assets/fonts/Roboto/Roboto-Bold.ttf"; */
  bool use_texture = false;

  // 900 is the initial width and height
  int width = 900, height = 900;
  // scale factors for the entire board to nicely fit as a square inside
  // the window
  float x_scale = 1.f, y_scale = 1.f;

  if (argc > 1) {
    char *flag = argv[1];
    if (strcmp(flag, "-h") == 0 || strcmp(flag, "--help") == 0) {
      usage();
      return 0;
    }

    for (int i = 1; i < argc; i++) {
      char *flag = argv[i];

      if (strcmp(flag, "-f") == 0 || strcmp(flag, "--font") == 0) {
        if (i + 1 < argc) {
          font_path = argv[i + 1];
          i++;
        } else {
          printf("Error: used font flag with no provided font, defaulting to Rubik\n");
        }
      } else if (strcmp(flag, "--use-texture") == 0) {
        use_texture = true;
      }
    }
  }

  int res = init_x11(&display, &window);
  if (res < 0) return 1;

  res = init_fonts(font_path);
  if (res == -1) {
    printf("Error: could not initialize freetype library\n");
    return 1;
  } else if (res == -2) {
    printf("Error: could not load font file: \"%s\"\n", font_path);
    return 1;
  }

  unsigned int font_vao, font_vbo;
  prepare_font(&font_vao, &font_vbo);
  Shader font_shader = create_shader("shaders/font_v.vert", "shaders/font_f.frag");

  Shader grid_shader;
  unsigned int grid_texture;
  if (!use_texture)
    grid_shader = create_shader("shaders/board_v.vert", "shaders/grid_f.frag");
  else
    grid_shader = create_shader("shaders/board_v.vert", "shaders/board_f.frag");
 
  int vao = prepare_bg(use_texture, &grid_texture);

  int board[9][9] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
  };
  srand(time(NULL));

  for (int i = 0; i < 9; i++) {
    for (int j = 0; j < 9; j++) {
      int r = (rand() % 9) + 1;
      board[i][j] = r;
    }
  }

  for (int i = 0; i < 9; i++) {
    for (int j = 0; j < 9; j++) {
      printf("%d | ", board[i][j]);
    }
    printf("\n");
  }
  printf("\n");

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
      } else if (xev.type == ButtonPress) {
        if (xev.xbutton.button == Button1) {
          int x = xev.xbutton.x;
          int y = xev.xbutton.y;
          // TOOD:
          printf("x: %d, y: %d\n", x, y);
        }
      }
    }

    glClearColor(0.2, 0.2, 0.2, 1.0);
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
      draw_bg_grid_texture(grid_shader, vao, grid_texture, (float *)transform);
    /* draw_numbers(font_shader, font_vao, font_vbo, (float *)transform, board); */
    draw_win_overlay(font_shader, font_vao, font_vbo, (float *)transform);

    glXSwapBuffers(display, window);
  }

  close_x11(&display, &window);

  return 0;
}

