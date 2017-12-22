#ifndef __VEHICLE_H__
#define __VEHICLE_H__

#include "cost.h"
#include <string>

class Route;
class Cost;
class Task;
class Plan;

class Vehicle
{
  std::string _name;
  CostMatrix::CityIdxType _avl_loc;
  const double _avl_time;  // hours
  Plan *_candidate_plans_sorted;
  const int _candidate_num_limit;
  // a self-loop virtual task/route leading in all possible plans
  Cost *_start_cost;
  Task *_start_task;
  Route *_start_route;
public:
  Vehicle(const char *name, CostMatrix::CityIdxType avl_loc,
          const double avl_time, const int candidate_num_limit);
  ~Vehicle();
};

#endif  // __VEHICLE_H__
