#include "scheduler.h"
#include "cost.h"
#include "timer.h"
#include <iostream>
#include <cstdlib>

#ifdef DEBUG
#include <omp.h>
omp_lock_t writelock;
#endif

using namespace std;

#ifdef DEBUG
extern const int vsp_debug = 1;
#else
extern const int vsp_debug = 0;
#endif  // DEBUG

extern int
multi_proc_main(Scheduler *sh);

static const char *_ORDER_FILE = "orders.http_api.json";
static const char *_VEHICLE_FILE = "vehicles.http_api.json";
static const char *_COST_PROB_FILE = "cost_prob.cc.json";
static const char *_OUTPUT_PLAN_FILE = "cxx_outplans.json";
static const char *_OUTPUT_VALUE_NETWORK_FILE = "cxx_value_network.json";

void
compute(Scheduler *sh)
{
  double t1 = get_wall_time();
  sh->run();
  print_wall_time_diff(t1, "Total Scheduler::run");

  t1 = get_wall_time();
  sh->dump_plans(_OUTPUT_PLAN_FILE);
  print_wall_time_diff(t1, "Total Scheduler::dump_plans");
  sh->dump_value_networks(_OUTPUT_VALUE_NETWORK_FILE);
  print_wall_time_diff(t1, "Total Scheduler::dump_value_networks");
}

namespace
{
  inline bool
    _is_empty(const char *s)
    {
      return !s || !s[0];
    }
  inline void
    _download(const char *api, const char *ofname)
    {
      string cmd = "curl --silent --show-error --output ";
      cmd += ofname;
      cmd += " ";
      cmd += api;
      system(cmd.c_str());
      cmd = "$" + cmd + ";";
      cout << cmd << endl;
    }
}

int NITER = 1;
int CITER = 1;

/*
 * **Usage**
 *
 * OpenMP multi-thread is turned on by default.
 *
 * Example 1 (locat testing with multi-process)
 * $ ./vsp 1 "" ""
 *
 * Example 2 (download but no upload)
 * $ ./vsp 1 "http://<order_api>?mark=..." "http://<vehicle_api>?mark=..."
 *
 * Example 3 (locat testing with single-process)
 * $ ./vsp "" "" ""
 *
 */
int main(int argc, char *argv[])
{
  cout << "Input number of iteration: ";
  cin >> NITER;
#ifdef DEBUG
  omp_init_lock(&writelock);
#endif
  double tg1 = get_wall_time();
  double t1 = get_wall_time();
  // arguement parsing
  if (argc <= 3)
  {
    cout << "** Error: too few arguements! Expect 3 arguements." << endl;
    return -1;
  }
  const bool multi_proc = ('1' == argv[1][0]);
  const char *order_api = !_is_empty(argv[2])? argv[2]: (const char *)0;
  const char *vehicle_api = !_is_empty(argv[3])? argv[3]: (const char *)0;
#pragma omp parallel sections
  {
    {
      if (!order_api)
        cout << "** Warning: Use order file due to no order API." << endl;
      else
        _download(order_api, _ORDER_FILE);
    }
#pragma omp section
    {
      if (!vehicle_api)
        cout << "** Warning: Use vehicle file due to no vehicle API." << endl;
      else
        _download(vehicle_api, _VEHICLE_FILE);
    }
  }
  print_wall_time_diff(t1, "Total order & vehicle download time");
  // start
  t1 = get_wall_time();
  // TODO: create cm and sh in child process to avoid duplicate memory usage
  // Generate this file using "$ python python/cost.py cost.json prob.json"
  CostMatrix cm(_COST_PROB_FILE);
  print_wall_time_diff(t1, "Total init CostMatrix");
  t1 = get_wall_time();
  Scheduler sh(_VEHICLE_FILE, _ORDER_FILE, cm);
  print_wall_time_diff(t1, "Total init Scheduler");
  // multi-process switcher
  if (multi_proc)
    multi_proc_main(&sh);
  else  // single process
    compute(&sh);
  print_wall_time_diff(tg1, "Total");
  return 0;
}
