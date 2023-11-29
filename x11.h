#pragma once

#include <stdbool.h>

#include <X11/Xlib.h>

int x11_init(Display **display, Window *window, const char* title, int window_width, int window_height);
void x11_close(Display **display, Window *window);
void x11_resize_window(Display *display, Window window);
void x11_toggle_fullscreen(bool fullscreen, Display *display, Window window);
void x11_get_screen_size(Display *display, int *width, int *height);
void x11_make_window_non_resizable(Display *display, Window window, int width, int height);
