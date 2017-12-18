#include "route.h"
#include "task.h"
#include "cost.h"
#include "consts.h"
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
}

double
Route::expense() const
{
  return _cost.expense();
}

double
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

#if 0
bool
Route::connect(Task &task,
               const CostMatrix &cost_prob_mat,
               const double max_wait_time=Consts::DOUBLE_INF,
               const double max_empty_run_distance=Consts::DOUBLE_INF)
{
  max_empty_run_time = task.expected_start_time() - expected_end_time();
  if (max_empty_run_time <= 0)
    return false;
  bool connected = false;
  // check out every possible route duration between current location and task
  empty_run_costs = cost_prob_mat.costs(_this_task.loc_to(), task.loc_from());
  // TODO: translate Python to C++
  for k, c in empty_run_costs.items():
        # check out every possible route duration between current location and task
        empty_run_costs = cost_prob_mat.costs(self._this_task.loc_to, task.loc_from)
        for k, c in empty_run_costs.items():
            wait_time = max_empty_run_time - c.duration
            empty_run_distance = c.distance
            if wait_time < 0 \
                    or wait_time > max_wait_time \
                    or empty_run_distance > max_empty_run_distance:
                continue
            # create an EmptyRunTask object if task can be connected via this route
            empty_run_start_time = self.expected_end_time + wait_time
            empty_run = \
                EmtpyRunTask(loc_start=self._this_task.loc_to, loc_end=task.loc_from, start_time=empty_run_start_time,
                             occur_prob=task.prob, is_virtual=task.is_virtual, wait_time=wait_time)
            empty_run_route = Route(task=empty_run, name=k, cost_obj=c)
            empty_run.add_route(empty_run_route)
            candidate_step = Step(empty_run_route=empty_run_route, order_task=task)
            self.add_next_step(candidate_step)
            connected = True
        return connected
}
#endif // 0
