#include "timer.h"

struct timeval start_time;

double get_time() {
  struct timeval current_time;
  gettimeofday(&current_time, NULL);

  return (double)(current_time.tv_sec - start_time.tv_sec) + (double)(current_time.tv_usec - start_time.tv_usec) / 1000000.0;
}

void start_internal_timer() {
  gettimeofday(&start_time, NULL);
}

bool timer_ended(Timer *timer) {
  if (!timer->is_running) return false;

  return get_time() - timer->time >= timer->duration;
}

void timer_start(Timer *timer, float duration) {
  timer->time = get_time();
  timer->duration = duration;
  timer->is_running = true;
}

void timer_stop(Timer *timer) {
  timer->is_running = false;
}

void timer_reset(Timer *timer) {
  timer->time = get_time();
  timer->is_running = true;
}

float timer_remaining(Timer *timer) {
  return timer->duration - (get_time() - timer->time);
}

float timer_elapsed(Timer *timer) {
  return get_time() - timer->time;
}
