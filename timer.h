#pragma once

#include <stdbool.h>

typedef enum TimerState {
  TIMER_RUNNING,
  TIMER_PAUSED,
  TIMER_STOPPED,
} TimerState;

typedef struct Timer {
  double start;
  double elapsed;
  float duration;
  TimerState state;
} Timer;

double get_time(void);
void start_internal_timer(void);
bool timer_ended(Timer *timer);
void timer_start(Timer *timer, float duration);
void timer_stop(Timer *timer);
void timer_reset(Timer *timer);
double timer_remaining(Timer *timer);
double timer_elapsed(Timer *timer);
void timer_pause(Timer *timer);
void timer_resume(Timer *timer);
