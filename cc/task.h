#ifndef __TASK_H__
#define __TASK_H__

#include "cost.h"
#include "consts.h"
#include <vector>
#include <string>

class Route;
class OrderTask;

class Task
{
  const CostMatrix::CityIdxType _location[2];  // [0]->start, [1]->end
  const double _expected_start_time;  // hours since 00:00:00 on 1970/1/1
  const double _occur_prob;  // occurrence probability
  std::vector<Route *>_routes;
  const bool _is_virtual;
  const std::string _name;
public:
  Task(const CostMatrix::CityIdxType loc_start,
       const CostMatrix::CityIdxType loc_end,
       const double start_time,
       const double occur_prob,
       const bool is_virtual,
       const std::string &name);
  virtual ~Task();
public:
  const std::string &
    name() const { return _name; }
  const CostMatrix::CityIdxType
    loc_from() const { return _location[0]; }
  const CostMatrix::CityIdxType
    loc_to() const { return _location[1]; }
  const double
    expected_start_time() const { return _expected_start_time; }
  const double
    prob() const { return _occur_prob; }
  std::vector<Route *> &
    routes() { return _routes; }
  virtual const double
    load_time() const { return 0.0; }
  virtual const double
    unload_time() const { return 0.0; }
  // a task may have no run time when it's waiting, loading or unloading
  virtual const double
    no_run_time() const { return 0.0; }
  virtual const double
    receivable() const { return 0.0; }
  virtual const double
    wait_time() const { return 0.0; }
  const bool
    is_virtual() const { return _is_virtual; }
  void
    add_route(Route *route) { _routes.push_back(route); }
  const bool
    connect(OrderTask &task,
            const CostMatrix &cost_prob_mat,
            const double max_wait_time=Consts::DOUBLE_INF,
            const double max_empty_run_distance=Consts::DOUBLE_INF);
};

class OrderTask: public Task
{
  const double _receivable;  // yuan
  const double _load_time;  // hours
  const double _unload_time;  // hours
  Route *_max_profit_route;
public:
  OrderTask(const CostMatrix::CityIdxType loc_start,
            const CostMatrix::CityIdxType loc_end,
            const double start_time,
            const double occur_prob,
            const bool is_virtual,
            const std::string &name,
            const double receivable,
            const double load_time,
            const double unload_time);
    ~OrderTask();
public:
    const double
      load_time() const { return _load_time; }
    const double
      unload_time() const { return _unload_time; }
    const double
      no_run_time() const { return _load_time + _unload_time; }
    const double
      receivable() const { return _receivable; }
    Route *
      max_profit_route();
};

class EmptyRunTask: public Task
{
  const double _wait_time;
public:
  EmptyRunTask(const CostMatrix::CityIdxType loc_start,
               const CostMatrix::CityIdxType loc_end,
               const double start_time,
               const double occur_prob=1.0,
               const bool is_virtual=false,
               const std::string &name="",
               const double wait_time=0.0);
  ~EmptyRunTask();
  const double
    wait_time() const { return _wait_time; }
  const double
    no_run_time() const { return wait_time(); }
};

#endif /* __TASK_H__ */
