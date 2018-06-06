#include "task.h"
#include "route.h"
#include <vector>
#include <algorithm>
#include <cassert>

#ifdef DEBUG
#include <iostream>
#include <omp.h>
extern omp_lock_t writelock;
#endif

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
              SchedulerMemBuf *smb,
              const size_t thread_id,
              const double max_wait_time,
              const double max_empty_run_distance)
{
  if (this == &task)
    return false;  // avoid connecting to itself
  bool connected = false;
  for (std::vector<Route *>::iterator it = _routes.begin();
       it != _routes.end(); ++it)
    connected = (*it)->connect(task, cost_prob_mat, smb, thread_id,
                               max_wait_time, max_empty_run_distance);
  return connected;
}

#ifdef DEBUG
size_t
OrderTask::_num_objs = 0;

void
OrderTask::print_num_objs()
{
  cout << "Total number of OrderTask objects: " << _num_objs << endl;
}
#endif

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
  _line_expense(line_expense), _max_net_value_route(0)
{
#ifdef DEBUG
  omp_set_lock(&writelock);
  ++_num_objs;
  omp_unset_lock(&writelock);
#endif
}

OrderTask::~OrderTask()
{
}

Route *
OrderTask::max_net_value_route()
{
  if (_max_net_value_route)
    return _max_net_value_route;
  // calculate max net value recursively
  vector<Route *> &rs = routes();
  assert(!rs.empty());
  for (vector<Route *>::iterator it = rs.begin(); it != rs.end(); ++it)
    (*it)->update_net_value();
  return _max_net_value_route =
    *max_element(rs.begin(), rs.end(), Route::cmp_net_value);
}

#ifdef DEBUG
size_t
EmptyRunTask::_num_objs = 0;

void
EmptyRunTask::print_num_objs()
{
  cout << "Total number of EmptyRunTask objects: " << _num_objs << endl;
}
#endif

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
#ifdef DEBUG
  omp_set_lock(&writelock);
  ++_num_objs;
  omp_unset_lock(&writelock);
#endif
}

EmptyRunTask::~EmptyRunTask()
{
}

