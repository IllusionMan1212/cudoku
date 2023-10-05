#include <stdio.h>
#include <string.h>
#include <time.h>

#include <glad/glx.h>
#include "3rdparty/stb/stb_image.h"

#include "x11.h"
#include "cudoku.h"
#include "font.h"
#include "helper.h"
#include "audio.h"

Display *display;
Window window;

void usage() {
  printf("Usage: cudoku [OPTION]\n\n");
  printf("  %-20s%-20s", "-h, --help", "prints this help message\n");
  printf("  %-20s%-20s", "--use-texture", "use a texture instead of generating the grid using shaders\n");
  printf("  %-20s%-20s", "-f, --font FONT", "use a custom font file to render text\n");
}

void handle_keypress(XEvent xev, Cudoku *game) {
  if (XLookupKeysym(&xev.xkey, 0) == XK_r) {
    reset_board(game);
  } else if (XLookupKeysym(&xev.xkey, 0) >= XK_0 && XLookupKeysym(&xev.xkey, 0) <= XK_9) {
    set_selected_number(game, XLookupKeysym(&xev.xkey, 0) - XK_0);
  } else if (XLookupKeysym(&xev.xkey, 0) == XK_BackSpace ||
      XLookupKeysym(&xev.xkey, 0) == XK_Delete) {
    set_selected_number(game, 0);
  } else if (
      XLookupKeysym(&xev.xkey, 0) == XK_Left ||
      XLookupKeysym(&xev.xkey, 0) == XK_a ||
      XLookupKeysym(&xev.xkey, 0) == XK_h) {
    move_selection(game, -1, 0);
  } else if (
      XLookupKeysym(&xev.xkey, 0) == XK_Right ||
      XLookupKeysym(&xev.xkey, 0) == XK_d ||
      XLookupKeysym(&xev.xkey, 0) == XK_l) {
    move_selection(game, 1, 0);
  } else if (
      XLookupKeysym(&xev.xkey, 0) == XK_Up ||
      XLookupKeysym(&xev.xkey, 0) == XK_w ||
      XLookupKeysym(&xev.xkey, 0) == XK_k) {
    move_selection(game, 0, -1);
  } else if (
      XLookupKeysym(&xev.xkey, 0) == XK_Down ||
      XLookupKeysym(&xev.xkey, 0) == XK_s ||
      XLookupKeysym(&xev.xkey, 0) == XK_j) {
    move_selection(game, 0, 1);
  } else if (
      XLookupKeysym(&xev.xkey, 0) == XK_Return ||
      XLookupKeysym(&xev.xkey, 0) == XK_space) {
    toggle_selection(game);
  } else if (XLookupKeysym(&xev.xkey, 0) == XK_n) {
    generate_random_board(game);
  } else if (XLookupKeysym(&xev.xkey, 0) == XK_F11) {
    toggle_fullscreen(display, window);
  } else if (XLookupKeysym(&xev.xkey, 0) == XK_c) {
    toggle_check(game);
  /* } else if (XLookupKeysym(&xev.xkey, 0) == XK_m) { */
  /*   toggle_mute(&game); */
  } else if (XLookupKeysym(&xev.xkey, 0) == XK_F1) {
    toggle_help(game);
  }
}

int main(int argc, char *argv[]) {
  int res = audio_init();
  if (res != 0) {
    printf("[ERROR]: failed to initialize audio\n");
    return 1;
  }

  const char *font_path = "assets/fonts/Rubik/Rubik-VariableFont_wght.ttf";
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
          printf("[ERROR]: used font flag with no provided font, defaulting to Rubik\n");
        }
      } else if (strcmp(flag, "--use-texture") == 0) {
        FILE *fp = fopen("assets/sudoku-grid.png", "r");
        if (fp == NULL) {
          printf("[ERROR]: could not open board texture, falling back to shader grid\n");
        } else {
          fclose(fp);
          use_texture = true;
        }
      }
    }
  }

  res = init_x11(&display, &window);
  if (res < 0) return 1;

  res = init_fonts(font_path);
  if (res == -1) {
    printf("[ERROR]: could not initialize freetype library\n");
    return 1;
  } else if (res == -2) {
    printf("[ERROR]: could not load font file: \"%s\"\n", font_path);
    return 1;
  }

  srand(time(NULL));

  Cudoku game = {0};
  game.should_draw_help = true;
  generate_random_board(&game);

  for (int i = 0; i < 9; i++) {
    for (int j = 0; j < 9; j++) {
      if (game.board[i][j].value) {
        printf("%d | ", game.board[i][j].value);
      } else {
        printf("  | ");
      }
    }
    printf("\n");
  }
  printf("\n");

  unsigned int font_vao, font_vbo;
  prepare_font(&font_vao, &font_vbo);
  Shader font_shader = create_shader("shaders/font_v.vert", "shaders/font_f.frag");

  Shader grid_shader;
  unsigned int grid_texture;
  if (!use_texture)
    grid_shader = create_shader("shaders/board_v.vert", "shaders/grid_f.frag");
  else
    grid_shader = create_shader("shaders/board_v.vert", "shaders/board_f.frag");
 
  int board_vao = prepare_bg(use_texture, &grid_texture);

  Shader selection_shader = create_shader("shaders/board_v.vert", "shaders/quad.frag");
  unsigned int selection_vao, selection_vbo;
  Color selection_color = {0.4f, 0.4f, 1.0f, 0.5f};
  prepare_selection_box(&selection_vao, &selection_vbo);

  Shader win_shader = create_shader("shaders/board_v.vert", "shaders/quad.frag");
  unsigned int win_vao = prepare_win_overlay();

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
        if (XLookupKeysym(&xev.xkey, 0) == XK_Escape ||
            (XLookupKeysym(&xev.xkey, 0) == XK_q && xev.xkey.state & ControlMask)) {
          quit = true;
          break;
        } else {
          handle_keypress(xev, &game);
        }
      } else if (xev.type == ButtonPress) {
        if (xev.xbutton.button == Button1) {
          int x = xev.xbutton.x;
          int y = xev.xbutton.y;
          do_selection(&game, x, y, width, height, x_scale, y_scale);
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

    Matrix4x4 projection = orthographic_projection_2d(0.0f, width, 0.0f, height);

    if (!use_texture)
      draw_bg_grid_shader(grid_shader, board_vao, (float *)transform, width < height ? width : height);
    else
      draw_bg_grid_texture(grid_shader, board_vao, grid_texture, (float *)transform);

    if (game.should_draw_selection)
      draw_selection_box(
          selection_shader,
          selection_vao,
          selection_vbo,
          game.selection.x,
          game.selection.y,
          (float *)transform,
          selection_color);

    draw_numbers(font_shader, font_vao, font_vbo, (float *)transform, game.board);

    if (game.should_highlight_mistakes)
      highlight_mistakes(selection_shader, selection_vao, selection_vbo, (float *)transform, &game);

    if (game.should_draw_help)
      draw_help_overlay(selection_shader, font_shader, selection_vao, selection_vbo, font_vao, font_vbo, projection, height);

    if (game.has_won) {
      draw_win_overlay(win_shader, font_shader, win_vao, font_vao, font_vbo, (float *)transform);
    }

    glXSwapBuffers(display, window);
    audio_update();
  }

  audio_cleanup();
  close_x11(&display, &window);

  return 0;
}

