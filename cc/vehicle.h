#ifndef __VEHICLE_H__
#define __VEHICLE_H__

#include "cost.h"
#include "consts.h"
#include <string>

class Route;
class Task;
class OrderTask;
class Plan;

class Vehicle
{
  std::string _name;
  CostMatrix::CityIdxType _avl_loc;
  const double _avl_time;  // hours
  const int _candidate_num_limit;
  std::vector<Plan *>_candidate_plans_sorted;
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
  static bool
    cmp(const Vehicle *va, const Vehicle *vb)
    { return va->avl_time() < vb->avl_time(); }
public:
  double
    avl_time() const { return _avl_time; }
  bool
    connect(OrderTask &task,
            const CostMatrix &cost_prob_mat,
            const double max_wait_time=Consts::DOUBLE_INF,
            const double max_empty_run_distance=Consts::DOUBLE_INF);
  void
    compute_max_profit();
  const std::vector<Plan *>&
    sorted_candidate_plans();
  cJSON *
    plans_to_dict(const CostMatrix &cost_prob_mat) const;
};

#endif  // __VEHICLE_H__

