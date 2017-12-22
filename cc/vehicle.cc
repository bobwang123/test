#include "vehicle.h"
#include "task.h"
#include "route.h"

Vehicle::Vehicle(const char *name, CostMatrix::CityIdxType avl_loc,
                 const double avl_time=0.0, const int candidate_num_limit=10)
  : _name(name), _avl_loc(avl_loc), _avl_time(avl_time),
  _candidate_num_limit(candidate_num_limit)
{
  _start_cost = new Cost();
  _start_task = new Task(_avl_loc, _avl_loc, _avl_time, 1.0, true, _name);
  _start_route = new Route(*_start_task, "", *_start_cost);
}

Vehicle::~Vehicle()
{
  delete[] _candidate_plans_sorted;
  delete _start_route;
  delete _start_task;
  delete _start_cost;
}
