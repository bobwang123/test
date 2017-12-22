#include "scheduler.h"
#include "cost.h"
#include "task.h"
#include "vehicle.h"
#include <cassert>

namespace
{
  // convert milliseconds to hours since 1970-1-1 00:00:00
  double _ms2hours(const double ms)
  {
    assert(ms > 0.0);
    return ms / 3600e3;
  }
}

Scheduler::Scheduler(const char *order_file, const char *vehicle_file,
                     const CostMatrix &cst_prb, void *opt)
  :_sorted_vehicles(0), _sorted_orders(0), _cost_prob(cst_prb), _opt(opt)
{
  if (!_init_vehicles_from_json(vehicle_file))
    _init_order_tasks_from_json(order_file);
}

Scheduler::~Scheduler()
{
  delete[] _sorted_vehicles;
  _sorted_vehicles = 0;
  delete[] _sorted_orders;
  _sorted_orders = 0;
}

int
Scheduler::_init_vehicles_from_json(const char *filename)
{
  return 0;
}

int
Scheduler::_init_order_tasks_from_json(const char *filename)
{
  return 0;
}

