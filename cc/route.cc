#include "route.h"
#include "task.h"
#include "consts.h"
#include "membuf.h"
#include <algorithm>
#include <cassert>

#include <iostream>
#ifdef DEBUG
#include <omp.h>
extern omp_lock_t writelock;

size_t
Route::_num_objs = 0;

void
Route::print_num_objs()
{
  std::cout << "Total number of Route objects: " << _num_objs << std::endl;
}
#endif

using namespace std;

Route::Route(Task &task, const string &name, const Cost &cost_obj)
  : _this_task(task), _name(name), _cost(&cost_obj),
  _net_value(Consts::DOUBLE_NONE)
{
  _expected_end_time = _this_task.expected_start_time() 
    + _this_task.no_run_time() + _cost->duration();
#ifdef DEBUG
  omp_set_lock(&writelock);
  ++_num_objs;
  omp_unset_lock(&writelock);
#endif
}

Route::~Route()
{
  for (std::vector<Step *>::iterator it = _next_steps.begin();
       it != _next_steps.end(); ++it)
    delete *it;
  for (std::list<EmptyRunTask *>::iterator it = _next_empty_task.begin();
       it != _next_empty_task.end(); ++it)
    delete *it;
}

const double
Route::gross_margin() const
{
  return _this_task.receivable() - expense();
}

const bool
Route::connect(OrderTask &task,
               const CostMatrix &cost_prob_mat,
               SchedulerMemBuf *smb,
               const size_t thread_id,
               const double max_wait_time,
               const double max_empty_run_distance)
{
  const double max_empty_run_time =
    task.expected_start_time() - expected_end_time();
  if (max_empty_run_time < 0)
    return false;
  bool connected = false;
  // check out every possible route duration between current location and task
  const CostMatrix::CostMapType &empty_run_costs =
    cost_prob_mat.costs(_this_task.loc_to(), task.loc_from());
  for (CostMatrix::CostMapType::const_iterator cit = empty_run_costs.begin();
       cit != empty_run_costs.end(); ++cit)
  {
    const Cost &c = cit->second;
    const double wait_time = max_empty_run_time - c.duration();
    const double empty_run_distance = c.distance();
    if (_this_task.is_virtual()
        && (wait_time < 0
            || wait_time > max_wait_time
            || empty_run_distance > max_empty_run_distance))
      continue;
    // create an EmptyRunTask object if task can be connected via this route
    const double empty_run_start_time = expected_end_time() + wait_time;
    EmptyRunTask *empty_run = smb ?
      new (smb->empty_run_task[thread_id]->allocate())
      EmptyRunTask(_this_task.loc_to(), task.loc_from(),
                   empty_run_start_time, task.prob(),
                   task.is_virtual(), "", wait_time):
      new
      EmptyRunTask(_this_task.loc_to(), task.loc_from(),
                   empty_run_start_time, task.prob(),
                   task.is_virtual(), "", wait_time);
    Route *empty_run_route = smb ?
      new (smb->route[thread_id]->allocate()) Route(*empty_run, cit->first, c):
      new Route(*empty_run, cit->first, c);
    empty_run->add_route(empty_run_route);
    Step *candidate_step = smb ?
      new (smb->step[thread_id]->allocate()) Step(*empty_run_route, task):
      new Step(*empty_run_route, task);
    add_next_step(candidate_step);
    connected = true;
    // record for later garbage collection
    _next_empty_task.push_back(empty_run);
  }
  return connected;
}

void Route::update_net_value()
{
  if (!Consts::is_none(_net_value))
    return;
  _net_value = gross_margin();
  if (_next_steps.empty())
    return;
  for (std::vector<Step *>::iterator it = _next_steps.begin();
       it != _next_steps.end(); ++it)
  {
    Step *ps = *it;
    if (Consts::is_none(ps->net_value()))
      ps->update_net_value();
  }
  stable_sort(_next_steps.begin(), _next_steps.end(), Step::cmp_net_value);
  /* Greedy strategy leads to the maximum math expectation
   * of all sub-Step net_values. */
  double sub_net_value = 0.0;
  for (std::vector<Step *>::iterator it = _next_steps.begin();
       it != _next_steps.end(); ++it)
  {
    const Step *ps = *it;
    const double p = ps->prob();
    sub_net_value = p * ps->net_value() + (1 - p) * sub_net_value;
  }
  _net_value += sub_net_value;
}

void
Route::reset_net_value_stuff()
{
  net_value(Consts::DOUBLE_NONE);
  std::vector<Step *> &steps = next_steps();
  for (std::vector<Step *>::iterator sit = steps.begin();
       sit !=steps.end(); ++sit)
    (*sit)->net_value(Consts::DOUBLE_NONE);
}

cJSON *
Route::to_dict(const CostMatrix &cost_prob_mat) const
{
  cJSON *route_dict = cJSON_CreateObject();
  cJSON_AddItemToObjectCS(route_dict, "orderId",
                          _this_task.name().empty() ? cJSON_CreateNull() :
                          cJSON_CreateString(_this_task.name().c_str()));
  cJSON_AddItemToObjectCS(route_dict, "orderMoney",
                          cJSON_CreateNumber(_this_task.receivable()));
  cJSON_AddItemToObjectCS(route_dict, "fromCity",
                          cJSON_CreateString(
                            cost_prob_mat.city_name(_this_task.loc_from())
                            .c_str()));
  cJSON_AddItemToObjectCS(route_dict, "toCity",
                          cJSON_CreateString(
                            cost_prob_mat.city_name(_this_task.loc_to())
                            .c_str()));
  cJSON_AddItemToObjectCS(route_dict, "orderedPickupTime",
                          cJSON_CreateNumber(round(
                              _this_task.expected_start_time() * 3600.0e3)));
  cJSON_AddItemToObjectCS(route_dict, "loadingTime",
                          cJSON_CreateNumber(_this_task.load_time()));
  cJSON_AddItemToObjectCS(route_dict, "unLoadingTime",
                          cJSON_CreateNumber(_this_task.unload_time()));
  cJSON_AddItemToObjectCS(route_dict, "isVirtual",
                          cJSON_CreateNumber(_this_task.is_virtual()));
  cJSON_AddItemToObjectCS(route_dict, "routeKey",
                          cJSON_CreateString(_name.c_str()));
  cJSON_AddItemToObjectCS(route_dict, "distance",
                          cJSON_CreateNumber(_cost->distance()));
  cJSON_AddItemToObjectCS(route_dict, "duration",
                          cJSON_CreateNumber(_cost->duration()));
  cJSON_AddItemToObjectCS(route_dict, "expense",
                          cJSON_CreateNumber(_cost->expense()));
  cJSON_AddItemToObjectCS(route_dict, "probability",
                          cJSON_CreateNumber(_this_task.prob()));
  cJSON_AddItemToObjectCS(route_dict, "waitingTime",
                          cJSON_CreateNumber(_this_task.wait_time()));
  cJSON_AddItemToObjectCS(route_dict, "lineCost",
                          Consts::is_none(_this_task.line_expense())?
                          cJSON_CreateNull():
                          cJSON_CreateNumber(_this_task.line_expense()));
  return route_dict;
}

cJSON *
Route::to_treemap(const int level, const double cond_prob,
                  const CostMatrix &cost_prob_mat,
                  const double empty_run_cost) const
{
  cJSON *route_treemap = cJSON_CreateObject();
  assert(empty_run_cost <= 0.0);
  const double step_net_value = net_value() + empty_run_cost;
  const double prob = _this_task.prob();
  enum {_NET_VALUE_EFF, _NET_VALUE, _PROB, _TIME_STAMP, _SIZE};
  const double value[_SIZE] = {
      // _NET_VALUE_EFF: contribution to this level
      [_NET_VALUE_EFF] = cond_prob * prob * step_net_value,
      [_NET_VALUE] = step_net_value,
      [_PROB] = prob,
      [_TIME_STAMP] = round(_this_task.expected_start_time() * 3600e3)
  };
  cJSON_AddItemToObjectCS(route_treemap, "value",
                          cJSON_CreateDoubleArray(value, _SIZE));
  string name = cost_prob_mat.city_name(_this_task.loc_from())
    + "->" + cost_prob_mat.city_name(_this_task.loc_to());
  cJSON_AddItemToObjectCS(route_treemap, "name",
                          cJSON_CreateString(name.c_str()));
  //cJSON_AddItemToObjectCS(route_treemap, "id", cJSON_CreateNull());
  if (_next_steps.empty())
    return route_treemap;
  if (level > 5)
    return route_treemap;
  cJSON *children = cJSON_CreateArray();
  // Greedy strategy
  double p = 1.0;
  const double PROB_TH = -10e-2;
  // dump limited nodes
  size_t c = 0;
#ifdef DEBUG
  if (level == 5)
  {
    std::cout << "[!]Level = " << level << ", children step size = " << _next_steps.size() << std::endl;
    std::cout << "[*] ";
    c = 0;
    for (std::vector<Step *>::const_reverse_iterator it = _next_steps.rbegin();
         it != _next_steps.rend() && p > PROB_TH; ++it, ++c)
    {
      const Step *ns = *it;
      if (!(c % 5))
      std::cout << ns->net_value() << " ";
    }
    std::cout << std::endl;
    getchar();
  }
#endif
  c = 0;
  for (std::vector<Step *>::const_reverse_iterator it = _next_steps.rbegin();
       it != _next_steps.rend() && p > PROB_TH; ++it, ++c)
  {
    const Step *ns = *it;
    if (!(c % 5))
      cJSON_AddItemToArray(children, ns->to_treemap(level + 1, p, cost_prob_mat));
    p *= 1.0 - ns->prob();
  }
  cJSON_AddItemToObjectCS(route_treemap, "children", children);
  return route_treemap;
}
