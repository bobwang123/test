#include "task.h"
#include "route.h"

Task::Task(CostMatrix::CityIdxType loc_start,
           CostMatrix::CityIdxType loc_end,
           double start_time,
           double occur_prob,
           bool is_virtual,
           const std::string &name)
  : _location(),
  _expected_start_time(start_time),
  _occur_prob(occur_prob),
  _routes(0),
  _is_virtual(is_virtual),
  _name(name)
{
  // cast const only in the constructor
  CostMatrix::CityIdxType *p = const_cast<CostMatrix::CityIdxType *>(_location);
  p[0] = loc_start;
  p[1] = loc_end;
}

Task::~Task()
{
  delete[] _routes;
  _routes = 0;
}

OrderTask::OrderTask(CostMatrix::CityIdxType loc_start,
                     CostMatrix::CityIdxType loc_end,
                     double start_time,
                     double occur_prob,
                     bool is_virtual,
                     const std::string &name,
                     double receivable,
                     double load_time,
                     double unload_time)
  : Task(loc_start, loc_end, start_time, occur_prob, is_virtual, name)
{
}

OrderTask::~OrderTask()
{
}
