#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <sys/time.h>
extern struct timeval start_time;

typedef struct Timer {
  float time;
  float duration;
  bool is_running;
} Timer;


double get_time();
void start_internal_timer();
bool timer_ended(Timer *timer);
void timer_start(Timer *timer, float duration);
void timer_reset(Timer *timer);
float timer_remaining(Timer *timer);
void timer_stop(Timer *timer);
float timer_elapsed(Timer *timer);
