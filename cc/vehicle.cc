#include "vehicle.h"
#include "task.h"
#include "route.h"
#include "cost.h"
#include "plan.h"
#include "step.h"
#include <iostream>
#include <algorithm>

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
  _candidate_plans_sorted.reserve(candidate_num_limit);
}

Vehicle::~Vehicle()
{
  for (vector<Plan *>::iterator it = _candidate_plans_sorted.begin();
       it != _candidate_plans_sorted.end(); ++it)
    delete *it;
  delete _start_route;
  _start_route = 0;
  delete _start_task;
  _start_task = 0;
  delete _start_cost;
  _start_cost = 0;
}

bool
Vehicle::connect(OrderTask &task,
                 const CostMatrix &cost_prob_mat,
                 SchedulerMemBuf *smb,
                 const size_t thread_id,
                 const double max_wait_time,
                 const double max_empty_run_distance)
{
  return _start_route->connect(task,
                               cost_prob_mat,
                               smb,
                               thread_id,
                               max_wait_time,
                               max_empty_run_distance);
}

void
Vehicle::compute_net_value()
{
  _start_route->update_net_value();
}

void
Vehicle::reset_net_value_stuff()
{
  _start_route->reset_net_value_stuff();
}

const vector<Plan *>&
Vehicle::sorted_candidate_plans()
{
  cout << "Output top " << _candidate_num_limit
    << " plans with the greatest net value." << endl;
  std::vector<Step *> &next_level1_steps = _start_route->next_steps();
  int c = 0;
  for (std::vector<Step *>::reverse_iterator rit = next_level1_steps.rbegin();
       rit != next_level1_steps.rend(); ++rit)
  {
    Step *step1 = *rit;
    if (step1->is_virtual())
      continue;
    if (c >= _candidate_num_limit)
      break;
    Plan *candidate_plan = new Plan;
    Step *step = step1;
    candidate_plan->append(step);
    while (!step->is_terminal())
    {
      step = step->next_net_value_step();
      candidate_plan->append(step);
    }
    _candidate_plans_sorted.push_back(candidate_plan);
    ++c;
  }
  return _candidate_plans_sorted;
}

cJSON *
Vehicle::plans_to_dict(const CostMatrix &cost_prob_mat) const
{
  cJSON *plans = cJSON_CreateArray();
  const size_t num_plans = _candidate_plans_sorted.size();
  for (size_t i = 0; i < num_plans; ++i)
    cJSON_AddItemReferenceToArray(
      plans, _candidate_plans_sorted.at(i)->to_dict(cost_prob_mat));
  cJSON *vehicle_plans = cJSON_CreateObject();
  cJSON_AddItemToObjectCS(
    vehicle_plans, "vehicleNo", cJSON_CreateString(_name.c_str()));
  cJSON_AddItemToObjectCS(vehicle_plans, "plans", plans);
  return vehicle_plans;
}

namespace
{
  inline bool _is_real_step(const Step *s)
  {
    return s && !s->is_virtual();
  }
}

cJSON *
Vehicle::to_treemap(const CostMatrix &cost_prob_mat) const
{
  vector<Step *> next_steps = _start_route->next_steps();
  // const Step *step1 = _start_route->next_steps().back();
  vector<Step *>::iterator step1_iter =
    find_if(next_steps.begin(), next_steps.end(), _is_real_step);
  if (next_steps.end() == step1_iter)  // no real step found
    return 0;
  cJSON *treemap = cJSON_CreateArray();
  cJSON_AddItemToArray(treemap,
                       (*step1_iter)->to_treemap(1, 1.0, cost_prob_mat));
  return treemap;
}
