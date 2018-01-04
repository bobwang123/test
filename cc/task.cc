#include "task.h"
#include "route.h"
#include <vector>
#include <algorithm>
#include <cassert>

using namespace std;

Task::Task(const CostMatrix::CityIdxType loc_start,
           const CostMatrix::CityIdxType loc_end,
           const double start_time,
           const double occur_prob,
           const bool is_virtual,
           const std::string &name)
  : _location(),
  _expected_start_time(start_time),
  _occur_prob(occur_prob),
  _is_virtual(is_virtual),
  _name(name)
{
  // init _location: cast const only in the constructor
  CostMatrix::CityIdxType *p = const_cast<CostMatrix::CityIdxType *>(_location);
  p[0] = loc_start;
  p[1] = loc_end;
  // speed up adding more _routes
  _routes.reserve(5);
}

Task::~Task()
{
  for (std::vector<Route *>::iterator it = _routes.begin();
       it != _routes.end(); ++it)
    delete *it;
}

const bool
Task::connect(OrderTask &task,
              const CostMatrix &cost_prob_mat,
              const double max_wait_time,
              const double max_empty_run_distance)
{
  if (this == &task)
    return false;  // avoid connecting to itself
  bool connected = false;
  for (std::vector<Route *>::iterator it = _routes.begin();
       it != _routes.end(); ++it)
    connected = (*it)->connect(
      task, cost_prob_mat, max_wait_time, max_empty_run_distance);
  return connected;
}


OrderTask::OrderTask(const CostMatrix::CityIdxType loc_start,
                     const CostMatrix::CityIdxType loc_end,
                     const double start_time,
                     const double occur_prob,
                     const bool is_virtual,
                     const std::string &name,
                     const double receivable,
                     const double load_time,
                     const double unload_time,
                     const double line_expense)
  : Task(loc_start, loc_end, start_time, occur_prob, is_virtual, name),
  _receivable(receivable), _load_time(load_time), _unload_time(unload_time),
  _line_expense(line_expense), _max_profit_route(0)
{
}

OrderTask::~OrderTask()
{
}

Route *
OrderTask::max_profit_route()
{
  if (_max_profit_route)
    return _max_profit_route;
  // calculate max profit recursively
  vector<Route *> &rs = routes();
  assert(!rs.empty());
  for (vector<Route *>::iterator it = rs.begin(); it != rs.end(); ++it)
    (*it)->update_max_profit();
  stable_sort(rs.begin(), rs.end(), Route::reverse_cmp);
  _max_profit_route = rs.front();
  return _max_profit_route;
}

EmptyRunTask::EmptyRunTask(const CostMatrix::CityIdxType loc_start,
                           const CostMatrix::CityIdxType loc_end,
                           const double start_time,
                           const double occur_prob,
                           const bool is_virtual,
                           const std::string &name,
                           const double wait_time)
  : Task(loc_start, loc_end, start_time, occur_prob, is_virtual, name),
  _wait_time(wait_time)
{
}

EmptyRunTask::~EmptyRunTask()
{
}

