#include "scheduler.h"
#include "cost.h"
#include "timer.h"
#include <iostream>

using namespace std;

#ifdef DEBUG
extern const int vsp_debug = 1;
#else
extern const int vsp_debug = 0;
#endif  // DEBUG

int main()
{
  double tg1 = get_wall_time();
  double t1 = get_wall_time();
  // Generate this file using "$ python python/cost.py cost.json prob.json"
  CostMatrix cm("cost_prob.cc.json");
  print_wall_time_diff(t1, "Total init CostMatrix");

  t1 = get_wall_time();
  Scheduler sh("vehicles.http_api.json", "orders.http_api.json", cm, 0);
  print_wall_time_diff(t1, "Total init Scheduler");

  t1 = get_wall_time();
  sh.run();
  print_wall_time_diff(t1, "Total Scheduler::run");

  t1 = get_wall_time();
  sh.dump_plans("cxx_outplans.json");
  print_wall_time_diff(t1, "Total Scheduler::dump_plans");

  print_wall_time_diff(tg1, "Total");
  return 0;
}
