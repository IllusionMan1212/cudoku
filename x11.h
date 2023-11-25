#pragma once

#include <X11/Xlib.h>

int init_x11(Display **display, Window *window, const char* title, int window_width, int window_height);
void close_x11(Display **display, Window *window);
void resize_x11_window(Display *display, Window window);
void toggle_fullscreen(Display *display, Window window);
void get_screen_size(Display *display, int *width, int *height);
void x11_make_window_non_resizable(Display *display, Window window, int width, int height);
