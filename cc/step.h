#ifndef __STEP_H__
#define __STEP_H__

#include "task.h"
#include "cJSON.h"

class Step
{
#ifdef DEBUG
  static size_t _num_objs;
#endif
  Route &_empty_run_route;
  OrderTask &_order_task;
  double _net_value;
public:
#ifdef DEBUG
  static void print_num_objs();
#endif
  Step(Route &empty_run_route, OrderTask &order_task);
  ~Step();
  static bool
    cmp_net_value(const Step *sa, const Step *sb)
    {
      // Avoid too sensitive sorting:
      // fixed cent vs. fixed cent, instead of yuan vs. yuan
      const long va = static_cast<long>(sa->net_value() * 100);
      const long vb = static_cast<long>(sb->net_value() * 100);
      return va < vb
      || ((va == vb)  // when fixed value equals
          && (sa->_order_task.expected_start_time()
              > sb->_order_task.expected_start_time()));
    }
public:
  const double
    prob() const { return _order_task.prob(); }
  const bool
    is_virtual() const { return _order_task.is_virtual(); }
  const double
    net_value() const { return _net_value; };
  void
    net_value(double v) { _net_value = v; };
  const double
    gross_margin() const;
  void
    update_net_value();
  Step *
    next_net_value_step();
  const bool
    is_terminal() const;
  cJSON *
    to_dict(const CostMatrix &cost_prob_mat) const;
  cJSON *
    to_treemap(const int level, const double cond_prob,
               const CostMatrix &cost_prob_mat) const;
};

#endif /* __STEP_H__ */
