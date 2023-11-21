#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <glad/glx.h>
#include "3rdparty/stb/stb_image.h"

#include "cudoku.h"
#include "ui.h"
#include "audio.h"

#define DEBUG 1

const char *font_path = "assets/fonts/Rubik/Rubik-VariableFont_wght.ttf";
const char *title = "Cudoku";

void usage() {
  printf("Usage: cudoku [OPTION]\n\n");
  printf("  %-20s%-20s", "-h, --help", "prints this help message\n");
  printf("  %-20s%-20s", "--use-texture", "use a texture instead of generating the grid using shaders\n");
  printf("  %-20s%-20s", "-f, --font FONT", "use a custom font file to render text\n");
}

void handle_keypress(XEvent xev, Cudoku *game) {
  if (XLookupKeysym(&xev.xkey, 0) == XK_r) {
    reset_board(game);
  } else if (
      xev.xkey.state & ControlMask &&
      xev.xkey.state & Mod1Mask &&
      XLookupKeysym(&xev.xkey, 0) == XK_w) {
#if DEBUG
    printf("DEBUG: Ctrl + Shift + W pressed. Winning game\n");

    game->has_won = true;
    game->should_draw_selection = false;
    game->should_highlight_mistakes = false;
    game->should_draw_help = false;
    game->win_time = get_time();
    timer_stop(&game->timer);

    audio_play_win();
#endif
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
    /* toggle_fullscreen(display, window); */
  } else if (XLookupKeysym(&xev.xkey, 0) == XK_c) {
    toggle_check(game);
  /* } else if (XLookupKeysym(&xev.xkey, 0) == XK_m) { */
  /*   toggle_mute(&game); */
  } else if (XLookupKeysym(&xev.xkey, 0) == XK_F1) {
    if (!toggle_help(game)) {
      timer_stop(&game->help_timer);
    }
  }
}

int main(int argc, char *argv[]) {
  bool use_texture = false;

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

  Color clear_color = {0.2f, 0.15f, 0.5f, 1.f};
  int res = init_zephr(font_path, title, (Size){800, 600}, &clear_color);
  if (res != 0) {
    printf("[ERROR]: could not initialize zephr\n");
    return 1;
  }

  srand(time(NULL));

  Cudoku game = {0};
  game.should_draw_help = true;
  generate_random_board(&game);

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

  float delta_t, last_t = 0.0;
  timer_start(&game.help_timer, 5.0f);
  timer_start(&game.timer, 0.0f);

  while (!zephr_should_quit()) {
    float now = get_time();
    delta_t = now - last_t;
    last_t = now;

    Size window_size = zephr_get_window_size();

    /* int width = 15.f; */
    /* int height = 35.f; */
    /* int padding = 10.f; */

    float ratio_w = (float)window_size.width / 1920.f;
    float ratio_h = (float)window_size.height / 1080.f;

    int width = window_size.height;
    int height = window_size.width;

    if (window_size.height > window_size.width) {
      width = window_size.width;
      height = window_size.width;
    } else {
      width = window_size.height;
      height = window_size.height;
    }

    draw_win();

    /* draw_quad( */
    /*     (Vec2){0.0, 0.0}, */
    /*     (Size){width, height}, */
    /*     &(Color){240.f, 235.f, 227.f, 255.f}, */
    /*     ALIGN_CENTER); */
    /* UIConstraints constraints = {0}; */
    /* set_x_constraint(&constraints, 0.0, UI_CONSTRAINT_FIXED); */
    /* set_y_constraint(&constraints, 0.0, UI_CONSTRAINT_FIXED); */
    /* set_width_constraint(&constraints, 50, UI_CONSTRAINT_FIXED); */
    /* set_height_constraint(&constraints, 50, UI_CONSTRAINT_FIXED); */

    /* draw_quad( */
    /*     constraints, */
    /*     &(Color){240.f, 235.f, 227.f, 255.f}, */
    /*     ALIGN_TOP_LEFT */
    /*     ); */

    /* Size size = calculate_text_size("Bruh", 1.0f); */
    /* float x = clampf(500.0f * timer_elapsed(&my_time), 0.0, window_size.width - size.width); */
    /* draw_text("Bruh", 1.0f, (Vec2){0.0, size.height}, &(Color){255.f, 0.f, 0.f, 255.f}); */

    zephr_swap_buffers();
  }
  /* while (!quit) { */

  /*   /1* while (XPending(display)) { *1/ */
  /*   /1*   XEvent xev; *1/ */
  /*   /1*   XNextEvent(display, &xev); *1/ */

  /*   /1*   if (xev.type == ConfigureNotify) { *1/ */
  /*   /1*     XConfigureEvent xce = xev.xconfigure; *1/ */

  /*   /1*     if (xce.width != width || xce.height != height) { *1/ */
  /*   /1*       width = xce.width; *1/ */
  /*   /1*       height = xce.height; *1/ */
  /*   /1*       resize_x11_window(display, window); *1/ */
  /*   /1*       set_scale_factor(xce.width, xce.height, &x_scale, &y_scale); *1/ */
  /*   /1*     } *1/ */

  /*   /1*   } else if (xev.type == KeyPress) { *1/ */
  /*   /1*     if (XLookupKeysym(&xev.xkey, 0) == XK_Escape || *1/ */
  /*   /1*         (XLookupKeysym(&xev.xkey, 0) == XK_q && xev.xkey.state & ControlMask)) { *1/ */
  /*   /1*       quit = true; *1/ */
  /*   /1*       break; *1/ */
  /*   /1*     } else { *1/ */
  /*   /1*       handle_keypress(xev, &game); *1/ */
  /*   /1*     } *1/ */
  /*   /1*   } else if (xev.type == ButtonPress) { *1/ */
  /*   /1*     if (xev.xbutton.button == Button1) { *1/ */
  /*   /1*       int x = xev.xbutton.x; *1/ */
  /*   /1*       int y = xev.xbutton.y; *1/ */
  /*   /1*       do_selection(&game, x, y, width, height, x_scale, y_scale); *1/ */
  /*   /1*     } *1/ */
  /*   /1*   } *1/ */
  /*   /1* } *1/ */

  /*   float transform[4][4] = { */
  /*     {x_scale, 0, 0, 0}, */
  /*     {0, y_scale, 0, 0}, */
  /*     {0, 0, 1, 0}, */
  /*     {0, 0, 0, 1}, */
  /*   }; */

  /*   /1* Matrix4x4 projection = orthographic_projection_2d(0.0f, width, 0.0f, height); *1/ */

  /*   /1* if (!use_texture) *1/ */
  /*   /1*   draw_bg_grid_shader(grid_shader, board_vao, (float *)transform, width < height ? width : height); *1/ */
  /*   /1* else *1/ */
  /*   /1*   draw_bg_grid_texture(grid_shader, board_vao, grid_texture, (float *)transform); *1/ */

  /*   if (game.should_draw_selection) */
  /*     draw_selection_box( */
  /*         selection_shader, */
  /*         selection_vao, */
  /*         selection_vbo, */
  /*         game.selection.x, */
  /*         game.selection.y, */
  /*         (float *)transform, */
  /*         selection_color); */

  /*   /1* draw_numbers(font_shader, font_vao, font_vbo, (float *)transform, game.board); *1/ */

  /*   /1* if (game.should_highlight_mistakes) { *1/ */
  /*   /1*   highlight_mistakes(selection_shader, selection_vao, selection_vbo, (float *)transform, &game); *1/ */

  /*   /1*   char* text = "Mistake highlighter is ON"; *1/ */
  /*   /1*   float text_scale = 0.2; *1/ */
  /*   /1*   Size text_size = calculate_text_size(text, text_scale); *1/ */
  /*   /1*   Vec2 text_pos = { 0.0f, text_size.height - 10.f }; *1/ */
  /*   /1*   Color text_color = { 200.0f, 50.0f, 50.0f, 1.0f }; *1/ */
  /*   /1*   draw_text_at(font_shader, text, text_pos, text_scale, font_vao, font_vbo, (float *)projection.m, &text_color); *1/ */
  /*   /1* } *1/ */

  /*   /1* draw_timer(font_shader, font_vao, font_vbo, (float *)projection.m, &game.timer, width); *1/ */

  /*   if (timer_ended(&game.help_timer)) { */
  /*     timer_stop(&game.help_timer); */
  /*     game.should_draw_help = false; */
  /*   } */

  /*   /1* if (game.should_draw_help) *1/ */
  /*   /1*   draw_help_overlay(selection_shader, font_shader, selection_vao, selection_vbo, font_vao, font_vbo, &projection, height, &game.help_timer); *1/ */

  /*   /1* if (game.has_won) { *1/ */
  /*   /1*   draw_win_overlay(win_shader, font_shader, win_vao, font_vao, font_vbo, (float *)transform, &game); *1/ */
  /*   /1* } *1/ */

  /*   /1* glXSwapBuffers(display, window); *1/ */
  /* } */

  deinit_zephr();

  return 0;
}

