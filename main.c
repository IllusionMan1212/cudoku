#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <X11/Xlib.h>

#include "audio.h"
#include "cudoku.h"
#include "timer.h"
#include "zephr.h"
#include "zephr_math.h"

#define DEBUG 1

const char *font_path = "assets/fonts/Rubik/Rubik-VariableFont_wght.ttf";
const char *title = "Cudoku";

void usage() {
  printf("Usage: cudoku [OPTION]\n\n");
  printf("  %-30s%-20s", "-h, --help", "prints this help message\n");
  printf("  %-30s%-20s", "-f, --font <path_to_font>", "use a custom font file to render text\n");
}

void handle_keypress(ZephrEvent e, Cudoku *game) {
  if (e.key.code == ZEPHR_KEYCODE_R) {
    reset_board(game);
  } else if (
      e.key.code == ZEPHR_KEYCODE_ESCAPE ||
      ((e.key.mods & ZEPHR_KEY_MOD_CTRL) && e.key.code == ZEPHR_KEYCODE_Q)
      ) {
    zephr_quit();
  } else if (
      e.key.mods & (ZEPHR_KEY_MOD_CTRL | ZEPHR_KEY_MOD_SHIFT) &&
      e.key.code == ZEPHR_KEYCODE_W) {
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
  } else if (e.key.code >= ZEPHR_KEYCODE_1 && e.key.code <= ZEPHR_KEYCODE_9) {
    set_selected_number(game, e.key.code - ZEPHR_KEYCODE_1 + 1);
  } else if (e.key.code == ZEPHR_KEYCODE_0) {
    set_selected_number(game, 0);
  } else if (e.key.code == ZEPHR_KEYCODE_BACKSPACE ||
      e.key.code == ZEPHR_KEYCODE_DELETE) {
    set_selected_number(game, 0);
  } else if (
      e.key.code == ZEPHR_KEYCODE_LEFT ||
      e.key.code == ZEPHR_KEYCODE_A ||
      e.key.code == ZEPHR_KEYCODE_H) {
    move_selection(game, -1, 0);
  } else if (
      e.key.code == ZEPHR_KEYCODE_RIGHT ||
      e.key.code == ZEPHR_KEYCODE_D ||
      e.key.code == ZEPHR_KEYCODE_L) {
    move_selection(game, 1, 0);
  } else if (
      e.key.code == ZEPHR_KEYCODE_UP ||
      e.key.code == ZEPHR_KEYCODE_W ||
      e.key.code == ZEPHR_KEYCODE_K) {
    move_selection(game, 0, -1);
  } else if (
      e.key.code == ZEPHR_KEYCODE_DOWN ||
      e.key.code == ZEPHR_KEYCODE_S ||
      e.key.code == ZEPHR_KEYCODE_J) {
    move_selection(game, 0, 1);
  } else if (
      e.key.code == ZEPHR_KEYCODE_ENTER ||
      e.key.code == ZEPHR_KEYCODE_SPACE) {
    toggle_selection(game);
  } else if (e.key.code == ZEPHR_KEYCODE_N) {
    generate_random_board(game);
  } else if (e.key.code == ZEPHR_KEYCODE_C) {
    toggle_check(game);
  } else if (e.key.code == ZEPHR_KEYCODE_P) {
    pause_game(game);
  } else if (e.key.code == ZEPHR_KEYCODE_F1) {
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
      char *option = argv[i];

      if (strcmp(option, "-f") == 0 || strcmp(option, "--font") == 0) {
        if (i + 1 < argc) {
          font_path = argv[i + 1];
          i++;
        } else {
          printf("[WARN]: Used font flag with no provided font, defaulting to Rubik\n");
        }
      }
    }
  }

  Size window_size = {900, 900};
  int res = init_zephr(font_path, title, window_size);
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
    ZephrEvent event;

    while (zephr_iter_events(&event)) {
      switch (event.type) {
        case ZEPHR_EVENT_UNKNOWN:
          printf("[WARN]: Unknown event\n");
          break;
        case ZEPHR_EVENT_KEY_PRESSED:
          handle_keypress(event, &game);
          break;
        case ZEPHR_EVENT_MOUSE_BUTTON_PRESSED:
          if (event.mouse.button == ZEPHR_MOUSE_BUTTON_LEFT) {
            do_selection(&game, event.mouse.position.x, event.mouse.position.y);
          }
          break;
        case ZEPHR_EVENT_WINDOW_CLOSED:
          zephr_quit();
          break;
        default:
        case ZEPHR_EVENT_WINDOW_RESIZED:
        case ZEPHR_EVENT_KEY_RELEASED:
          break;
        }
    }

    draw_board(&game, window_size);
    draw_timer(&game.timer);

    if (game.should_draw_selection) {
      draw_selection_box(game.selection.y, game.selection.x, (Color){102, 102, 255, 127});
    }

    if (game.should_highlight_mistakes) {
      draw_mistakes_highlight(&game);
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

