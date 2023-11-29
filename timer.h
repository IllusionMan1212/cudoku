#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <sys/time.h>

typedef enum TimerState {
  TIMER_RUNNING,
  TIMER_PAUSED,
  TIMER_STOPPED,
} TimerState;

typedef struct Timer {
  float start;
  float elapsed;
  float duration;
  TimerState state;
} Timer;

double get_time();
void start_internal_timer();
bool timer_ended(Timer *timer);
void timer_start(Timer *timer, float duration);
void timer_stop(Timer *timer);
void timer_reset(Timer *timer);
float timer_remaining(Timer *timer);
float timer_elapsed(Timer *timer);
void timer_pause(Timer *timer);
void timer_resume(Timer *timer);
