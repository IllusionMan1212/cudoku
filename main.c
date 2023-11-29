#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <glad/glx.h>
#include <stb_image.h>

#include "cudoku.h"
#include "audio.h"
#include "zephr.h"

#define DEBUG 1

extern Display *display;
const char *font_path = "assets/fonts/Rubik/Rubik-VariableFont_wght.ttf";
const char *title = "Cudoku";

void usage() {
  printf("Usage: cudoku [OPTION]\n\n");
  printf("  %-20s%-20s", "-h, --help", "prints this help message\n");
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
  } else if (XLookupKeysym(&xev.xkey, 0) == XK_c) {
    toggle_check(game);
  } else if (XLookupKeysym(&xev.xkey, 0) == XK_p) {
    pause_game(game);
  } else if (XLookupKeysym(&xev.xkey, 0) == XK_F1) {
    if (!toggle_help(game)) {
      timer_stop(&game->help_timer);
    }
  }
}

int main(int argc, char *argv[]) {
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
          printf("[WARN]: Used font flag with no provided font, defaulting to Rubik\n");
        }
      }
    }
  }

  Color clear_color = {0.0f, 0.0f, 0.0f, 1.f};
  Size window_size = {900, 900};
  int res = init_zephr(font_path, title, window_size, &clear_color);
  if (res != 0) {
    printf("[ERROR]: could not initialize zephr\n");
    return 1;
  }
  zephr_make_window_non_resizable();

  srand(time(NULL));

  Cudoku game = {0};
  game.should_draw_help = true;
  generate_random_board(&game);

  timer_start(&game.help_timer, 5.0f);
  timer_start(&game.timer, 0.0f);

  while (!zephr_should_quit()) {
    while (XPending(display)) {
      XEvent xev;
      XNextEvent(display, &xev);

      if (xev.type == KeyPress) {
        if (XLookupKeysym(&xev.xkey, 0) == XK_Escape ||
            (XLookupKeysym(&xev.xkey, 0) == XK_q && xev.xkey.state & ControlMask)) {
          zephr_quit();
          break;
        } else {
          handle_keypress(xev, &game);
        }
      } else if (xev.type == ButtonPress) {
        if (xev.xbutton.button == Button1) {
          int x = xev.xbutton.x;
          int y = xev.xbutton.y;
          do_selection(&game, x, y);
        }
      }
    }

    // TODO: how do I want the input handling to look like
    //zephr_poll_events();?
    //for event in zephr_events()??
    //

    draw_board(&game, window_size);
    draw_timer(&game.timer);

    if (game.should_draw_selection) {
      draw_selection_box(game.selection.y, game.selection.x, (Color){102, 102, 255, 127});
    }

    if (game.should_highlight_mistakes) {
      highlight_mistakes(&game);
    }

    if (timer_ended(&game.help_timer)) {
      timer_stop(&game.help_timer);
      game.should_draw_help = false;
    }

    if (game.should_draw_help) {
      draw_help(&game.help_timer);
    }

    if (game.has_won) {
      draw_win(&game);
    }

    if (game.timer.state == TIMER_PAUSED) {
      draw_pause_overlay();
    }

    zephr_swap_buffers();
  }

  deinit_zephr();

  return 0;
}

