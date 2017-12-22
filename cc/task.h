#ifndef __TASK_H__
#define __TASK_H__

#include "cost.h"
#include <string>

class Route;

class Task
{
  const CostMatrix::CityIdxType _location[2];  // [0]->start, [1]->end
  const double _expected_start_time;  // hours since 00:00:00 on 1970/1/1
  const double _occur_prob;  // occurrence probability
  Route *_routes;
  bool _is_virtual;
  const std::string _name;
public:
  Task(CostMatrix::CityIdxType loc_start,
       CostMatrix::CityIdxType loc_end,
       double start_time,
       double occur_prob,
       bool is_virtual,
       const std::string &name);
  ~Task();
public:
  double receivable() const { return 0.0; }
  double prob() const { return 0.0; }
  double expected_start_time() const { return 0.0; }
  double no_run_time() const { return 0.0; }
  bool is_virtual() const { return true; }
};

class OrderTask: public Task
{
public:
  OrderTask(CostMatrix::CityIdxType loc_start,
            CostMatrix::CityIdxType loc_end,
            double start_time,
            double occur_prob,
            bool is_virtual,
            const std::string &name,
            double receivable,
            double load_time,
            double unload_time);
    ~OrderTask();
};

#endif /* __TASK_H__ */
