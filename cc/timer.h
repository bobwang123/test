#ifndef __TIMER_H__
#define __TIMER_H__

double get_wall_time();

void print_wall_time_diff(const double start, const char *msg);

double get_cpu_time();

#endif  // __TIMER_H__
