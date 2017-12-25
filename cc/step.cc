#include "step.h"
#include "route.h"
#include "consts.h"
#include <cassert>

Step::Step(Route &empty_run_route, OrderTask &order_task)
  :_empty_run_route(empty_run_route), _order_task(order_task),
  _max_profit(Consts::DOUBLE_NONE)
{
}

Step::~Step()
{
}

const double
Step::profit() const
{
  return _empty_run_route.profit() + _order_task.max_profit_route()->profit();
}

void
Step::update_max_profit()
{
  if (!Consts::is_none(_max_profit))
    return;
  Route *order_route = _order_task.max_profit_route();
  assert(order_route);
  // update max profit recursively if not done yet
  if (Consts::is_none(order_route->max_profit()))
    order_route->update_max_profit();
  _max_profit = _empty_run_route.profit() + order_route->max_profit();
}

Step *
Step::next_max_profit_step()
{
  if (is_terminal())
    return 0;
  return _order_task.max_profit_route()->next_steps().front();
}

const bool
Step::is_terminal() const
{
  return _order_task.max_profit_route()->is_terminal();
}

cJSON *
Step::to_dict(const CostMatrix &cost_prob_mat) const
{
  cJSON *arr = cJSON_CreateArray();
  cJSON_AddItemToArray(
    arr, _empty_run_route.to_dict(cost_prob_mat));
  cJSON_AddItemToArray(
    arr, _order_task.max_profit_route()->to_dict(cost_prob_mat));
  return arr;
}

