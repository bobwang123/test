#include "route.h"
#include "task.h"
#include "consts.h"
#include <algorithm>
#include <cassert>

using namespace std;

Route::Route(Task &task, const string &name, const Cost &cost_obj)
  : _this_task(task), _name(name), _cost(cost_obj)
{
  _expected_end_time = _this_task.expected_start_time() 
    + _this_task.no_run_time() + _cost.duration();
  _profit = Consts::DOUBLE_NONE;
  _max_profit = Consts::DOUBLE_NONE;
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
Route::profit()
{
  if (Consts::is_none(_profit))
  {
    _profit = _this_task.receivable() - expense();
    if (_this_task.is_virtual())
      _profit *= _this_task.prob();
  }
  return _profit;
}

const bool
Route::connect(OrderTask &task,
               const CostMatrix &cost_prob_mat,
               const double max_wait_time,
               const double max_empty_run_distance)
{
  const double max_empty_run_time =
    task.expected_start_time() - expected_end_time();
  if (max_empty_run_time <= 0)
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
    if (wait_time < 0
        || wait_time > max_wait_time
        || empty_run_distance > max_empty_run_distance)
      continue;
    // create an EmptyRunTask object if task can be connected via this route
    const double empty_run_start_time = expected_end_time() + wait_time;
    EmptyRunTask *empty_run =
      new EmptyRunTask(_this_task.loc_to(), task.loc_from(),
                       empty_run_start_time, task.prob(),
                       task.is_virtual(), "", wait_time);
    Route *empty_run_route = new Route(*empty_run, cit->first, c);
    empty_run->add_route(empty_run_route);
    Step *candidate_step = new Step(*empty_run_route, task);
    add_next_step(candidate_step);
    connected = true;
    // record for later garbage collection
    _next_empty_task.push_back(empty_run);
  }
  return connected;
}

void
Route::update_max_profit()
{
  if (!Consts::is_none(_max_profit))
    return;
  if (_next_steps.empty())
  {
    _max_profit = profit();
    return;
  }
  for (std::vector<Step *>::iterator it = _next_steps.begin();
       it != _next_steps.end(); ++it)
    if (Consts::is_none((*it)->max_profit()))
      (*it)->update_max_profit();
  sort(_next_steps.begin(), _next_steps.end(), Step::reverse_cmp);
  Step *max_profit_step = _next_steps.at(0);
  assert(max_profit_step);
  _max_profit = profit();
  if (max_profit_step->max_profit() > 0.0)
    _max_profit += _this_task.prob() * max_profit_step->max_profit();
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
                          cJSON_CreateNumber(_this_task.expected_start_time()
                                             * 3600.0));
  cJSON_AddItemToObjectCS(route_dict, "loadingTime",
                          cJSON_CreateNumber(_this_task.load_time()));
  cJSON_AddItemToObjectCS(route_dict, "unLoadingTime",
                          cJSON_CreateNumber(_this_task.unload_time()));
  cJSON_AddItemToObjectCS(route_dict, "isVirtual",
                          cJSON_CreateBool(_this_task.is_virtual()));
  cJSON_AddItemToObjectCS(route_dict, "routeKey",
                          cJSON_CreateString(_name.c_str()));
  cJSON_AddItemToObjectCS(route_dict, "distance",
                          cJSON_CreateNumber(_cost.distance()));
  cJSON_AddItemToObjectCS(route_dict, "duration",
                          cJSON_CreateNumber(_cost.duration()));
  cJSON_AddItemToObjectCS(route_dict, "expense",
                          cJSON_CreateNumber(_cost.expense()));
  cJSON_AddItemToObjectCS(route_dict, "probability",
                          cJSON_CreateNumber(_this_task.prob()));
  cJSON_AddItemToObjectCS(route_dict, "waitingTime",
                          cJSON_CreateNumber(_this_task.wait_time()));
  return route_dict;
}

