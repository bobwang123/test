#ifndef __VEHICLE_H__
#define __VEHICLE_H__

#include "cost.h"
#include <string>

class Route;
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
  Vehicle(const std::string &name,
          CostMatrix::CityIdxType avl_loc,
          const double avl_time = 0.0,
          const int candidate_num_limit = 10);
  ~Vehicle();
};

#endif  // __VEHICLE_H__

