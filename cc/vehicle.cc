#include "vehicle.h"
#include "task.h"
#include "route.h"
#include "plan.h"

using namespace std;

Vehicle::Vehicle(const string &name,
                 const CostMatrix::CityIdxType avl_loc,
                 const double avl_time,
                 const int candidate_num_limit)
  : _name(name), _avl_loc(avl_loc), _avl_time(avl_time),
  _candidate_num_limit(candidate_num_limit)
{
  _start_cost = new Cost();
  _start_task = new Task(_avl_loc, _avl_loc, _avl_time, 1.0, true, _name);
  _start_route = new Route(*_start_task, "", *_start_cost);
}

Vehicle::~Vehicle()
{
  // delete[] _candidate_plans_sorted;
  _candidate_plans_sorted = 0;
  delete _start_route;
  _start_route = 0;
  delete _start_task;
  _start_task = 0;
  delete _start_cost;
  _start_cost = 0;
}

