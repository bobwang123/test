#include "scheduler.h"
#include "cost.h"

#ifdef DEBUG
extern const int vsp_debug = 1;
#else
extern const int vsp_debug = 0;
#endif  // DEBUG

int main()
{
  // Generate this file using "$ python python/cost.py cost.json prob.json"
  CostMatrix cm("cost_prob.cc.json");
  Scheduler sh("vehicles.http_api.json", "orders.http_api.json", cm, 0);
  return 0;
}
