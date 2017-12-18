#ifndef __ROUTE_H__
#define __ROUTE_H__

#include <vector>
#include <string>

class Cost;
class CostMatrix;
class Step;
class Task;

class Route
{
  Task &_this_task;  // this route belongs to _this_task
  const std::string _name;
  const Cost &_cost;
  double _expected_end_time;
  std::vector<Step *> _next_steps;  // all next reachable steps
  double _profit;  // profit of this route
  double _max_profit;  // max profit of all plans starting from this route
public:
  Route(Task &task, const std::string &name, const Cost &cost_obj);
  ~Route();
public:
  inline const std::string &name() const { return _name; }
  inline double expected_end_time() const { return _expected_end_time; }
  inline std::vector<Step *> &next_steps() { return _next_steps; }
  double expense() const;
public:
  double profit();
  bool connect(Task &task,
               const CostMatrix &cost_prob_mat,
               const double max_wait_time,
               const double max_empty_run_distance);
};

#endif /* __ROUTE_H__ */
