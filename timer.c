#include <sys/time.h>
#include <stdlib.h>

#include "timer.h"

struct timeval start_time;

double get_time(void) {
  struct timeval current_time;
  gettimeofday(&current_time, NULL);

  return (double)(current_time.tv_sec - start_time.tv_sec) + (double)(current_time.tv_usec - start_time.tv_usec) / 1000000.0;
}

void start_internal_timer(void) {
  gettimeofday(&start_time, NULL);
}

bool timer_ended(Timer *timer) {
  if (timer->state == TIMER_STOPPED) return false;

  return get_time() - timer->start >= timer->duration;
}

void timer_start(Timer *timer, float duration) {
  timer->start = get_time();
  timer->elapsed = 0;
  timer->duration = duration;
  timer->state = TIMER_RUNNING;
}

void timer_stop(Timer *timer) {
  timer->state = TIMER_STOPPED;
}

void timer_reset(Timer *timer) {
  timer->start = get_time();
  timer->elapsed = 0;
  timer->state = TIMER_RUNNING;
}

double timer_remaining(Timer *timer) {
  return timer->duration - (get_time() - timer->start);
}

double timer_elapsed(Timer *timer) {
  return get_time() - timer->start + timer->elapsed;
}

void timer_pause(Timer *timer) {
  timer->state = TIMER_PAUSED;
  timer->elapsed = get_time() - timer->start + timer->elapsed;
}

void timer_resume(Timer *timer) {
  timer->state = TIMER_RUNNING;
  timer->start = get_time();
}
