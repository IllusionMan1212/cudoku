#pragma once

#include <X11/Xlib.h>

int init_x11(Display **display, Window *window, const char* title, int window_width, int window_height);
void close_x11(Display **display, Window *window);
void resize_x11_window(Display *display, Window window);
void toggle_fullscreen(Display *display, Window window);
