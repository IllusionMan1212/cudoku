#pragma once

#include <X11/Xlib.h>

void resize_x11_window(Display *display, Window window);
int init_x11(Display **display, Window *window);
void close_x11(Display **display, Window *window);
void toggle_fullscreen(Display *display, Window window);
