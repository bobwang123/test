#ifndef __STEP_H__
#define __STEP_H__

#include "task.h"
#include "cJSON.h"

class Step
{
  Route &_empty_run_route;
  OrderTask &_order_task;
  double _max_profit;
public:
  Step(Route &empty_run_route, OrderTask &order_task);
  ~Step();
  static bool
    reverse_cmp(const Step *sa, const Step *sb)
    { return sb->max_profit() < sa->max_profit(); }
public:
  const double
    prob() const { return _order_task.prob(); }
  const double
    profit() const;
  const bool
    is_virtual() const { return _order_task.is_virtual(); }
  const double
    max_profit() const { return _max_profit; }
  void
    update_max_profit();
  Step *
    next_max_profit_step();
  const bool
    is_terminal() const;
  cJSON *
    to_dict(const CostMatrix &cost_prob_mat) const;
};

#endif /* __STEP_H__ */
