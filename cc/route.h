#ifndef __ROUTE_H__
#define __ROUTE_H__

#include "cost.h"
#include "step.h"
#include "consts.h"
#include <vector>
#include <list>
#include <string>

class SchedulerMemBuf;

class Route
{
#ifdef DEBUG
  static size_t _num_objs;
#endif
  Task &_this_task;  // this route belongs to _this_task
  const std::string _name;
  const Cost *_cost;
  double _expected_end_time;
  std::vector<Step *> _next_steps;  // all next reachable steps
  double _net_value;  // network value of this route
  std::list<EmptyRunTask *> _next_empty_task;  // for garbage collection
public:
#ifdef DEBUG
  static void print_num_objs();
#endif
  Route(Task &task, const std::string &name, const Cost &cost_obj);
  ~Route();
  static bool
    cmp_net_value(const Route *ra, const Route *rb)
    { return ra->net_value() < rb->net_value()
      || ((ra->net_value() == rb->net_value())
          && (ra->_this_task.expected_start_time()
              > rb->_this_task.expected_start_time())); }
public:
  const std::string &
    name() const { return _name; }
  const double
    expense() const { return _cost->expense(); }
  const double
    gross_margin() const;
  std::vector<Step *> &
    next_steps() { return _next_steps; }
  const bool
    connect(OrderTask &task,
            const CostMatrix &cost_prob_mat,
            SchedulerMemBuf *smb = 0,
            const size_t thread_id = 0,
            const double max_wait_time=Consts::DOUBLE_INF,
            const double max_empty_run_distance=Consts::DOUBLE_INF);
  void
    update_net_value();
  const double
    net_value() const { return _net_value; }
  const double
    expected_end_time() const { return _expected_end_time; }
  const bool
    is_terminal() const
    {
      if (Consts::is_none(net_value()))
        return false;
      return _next_steps.empty()
        || (_next_steps.back()->net_value() <= 0);
    }
  void
    add_next_step(Step *step) { _next_steps.push_back(step); }
  cJSON *
    to_dict(const CostMatrix &cost_prob_mat) const;
  cJSON *
    to_treemap(const int level, const double cond_prob,
               const CostMatrix &cost_prob_mat) const;
};

#endif /* __ROUTE_H__ */
