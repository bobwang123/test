#include "scheduler.h"
#include "cost.h"
#include "timer.h"
#include <iostream>
#include <cstdlib>

using namespace std;

#ifdef DEBUG
extern const int vsp_debug = 1;
#else
extern const int vsp_debug = 0;
#endif  // DEBUG

extern int
multi_proc_main(const char *api, Scheduler *sh);

static const char *_ORDER_FILE = "orders.http_api.json";
static const char *_VEHICLE_FILE = "vehicles.http_api.json";
static const char *_COST_PROB_FILE = "cost_prob.cc.json";
static const char *_OUTPUT_PLAN_FILE = "cxx_outplans.json";

void
compute(Scheduler *sh)
{
  double t1 = get_wall_time();
  sh->run();
  print_wall_time_diff(t1, "Total Scheduler::run");

  t1 = get_wall_time();
  sh->dump_plans(_OUTPUT_PLAN_FILE);
  print_wall_time_diff(t1, "Total Scheduler::dump_plans");
}

void
upload(const char *api)
{
  if (!api)
    return;
  string cmd = "curl --output curl.log --include --silent --show-error --form \"resultStr=<";
  cmd += _OUTPUT_PLAN_FILE;
  cmd += ";type=text/plain\" ";
  cmd += api;
  // TODO: system(cmd.c_str());
  cout << "$" << cmd << ";" << endl;
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
      // TODO: system(cmd.c_str());
      cmd = "$" + cmd + ";";
      cout << cmd << endl;
    }
}

/*
 * **Usage**
 *
 * OpenMP multi-thread is turned on by default.
 *
 * Example 1 (locat testing with multi-process)
 * $ ./vsp 1 "" "" ""
 *
 * Example 2 (download but no upload)
 * $ ./vsp 1 "http://<order_api>?mark=..." "http://<vehicle_api>?mark=..." ""
 *
 * Example 3 (download and upload)
 * $ ./vsp 1 "http://<order_api>?mark=..." "http://<vehicle_api>?mark=..." \
 *      "http://<upload_api>?mark=..."
 *
 * Example 4 (locat testing with single-process)
 * $ ./vsp "" "" "" ""
 *
 */
int main(int argc, char *argv[])
{
  // arguement parsing
  if (argc <= 4)
  {
    cout << "** Error: too few arguements! Expect 4 arguements." << endl;
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
  const char *upload_api = !_is_empty(argv[4])? argv[4]: (const char *)0;
  if (!upload_api)
    cout << "** Warning: No plan will be uploaded due to no API." << endl;
  // start
  double tg1 = get_wall_time();
  double t1 = get_wall_time();
  // TODO: create cm and sh in child process to avoid duplicate memory usage
  // Generate this file using "$ python python/cost.py cost.json prob.json"
  CostMatrix cm(_COST_PROB_FILE);
  print_wall_time_diff(t1, "Total init CostMatrix");
  t1 = get_wall_time();
  Scheduler sh(_VEHICLE_FILE, _ORDER_FILE, cm);
  print_wall_time_diff(t1, "Total init Scheduler");
  // multi-process switcher
  if (multi_proc)
  {
    multi_proc_main(upload_api, &sh);
  }
  else  // single process
  {
    compute(&sh);
    upload(upload_api);
  }
  print_wall_time_diff(tg1, "Total");
  return 0;
}
