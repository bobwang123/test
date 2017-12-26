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
  string cmd = "curl --output curl.log --include --silent --show-error --form resultsStr=<";
  cmd += _OUTPUT_PLAN_FILE;
  cmd += ";type=text/plain ";
  cmd += api;
  // TODO: system(cmd.c_str());
  cout << cmd << endl;
}

int main(int argc, char *argv[])
{
  const char *upload_api = argc > 2 ? argv[2] : static_cast<const char *>(0);
  if (!upload_api)
    cout << "** Warning: No plan is uploaded because API is invalid." << endl;
  double tg1 = get_wall_time();
  double t1 = get_wall_time();
  // Generate this file using "$ python python/cost.py cost.json prob.json"
  CostMatrix cm("cost_prob.cc.json");
  print_wall_time_diff(t1, "Total init CostMatrix");

  t1 = get_wall_time();
  Scheduler sh("vehicles.http_api.json", "orders.http_api.json", cm);
  print_wall_time_diff(t1, "Total init Scheduler");

  if (argc > 1 && argv[1][0] == '1')
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
