#include "scheduler.h"
#include "cost.h"
#include "task.h"
#include "vehicle.h"
#include "fileio.h"
#include <cassert>

namespace
{
  // convert milliseconds to hours since 1970-1-1 00:00:00
  double _ms2hours(const double ms)
  {
    assert(ms > 0.0);
    return ms / 3600e3;
  }
}

Scheduler::Scheduler(const char *vehicle_file,
                     const char *order_file,
                     const CostMatrix &cst_prb,
                     void *opt)
  :_sorted_vehicles(0),
  _num_vehicles(0),
  _sorted_orders(0),
  _num_orders(0),
  _cost_prob(cst_prb),
  _opt(opt)
{
  if (!_init_vehicles_from_json(vehicle_file))
    _init_order_tasks_from_json(order_file);
}

Scheduler::~Scheduler()
{
  for (register int i = 0; i < _num_vehicles; ++i)
    delete _sorted_vehicles[i];
  delete[] _sorted_vehicles;
  _sorted_vehicles = 0;
  for (register int i = 0; i < _num_orders; ++i)
    delete _sorted_orders[i];
  delete[] _sorted_orders;
  _sorted_orders = 0;
}

int
Scheduler::_init_vehicles_from_json(const char *filename)
{
  cJSON *json = parse_json_file(filename);
  cJSON *json_array_vehicles = cJSON_GetObjectItem(json, "data");
  _num_vehicles = cJSON_GetArraySize(json_array_vehicles);
  _sorted_vehicles = new Vehicle *[_num_vehicles];
  for (int i = 0; i < _num_vehicles; ++i)
  {
    cJSON *json_vehicle = cJSON_GetArrayItem(json_array_vehicles, i);
    const char *name =
      cJSON_GetObjectItem(json_vehicle, "vehicleNo")->valuestring;
    const char *avl_city_name =
      cJSON_GetObjectItem(json_vehicle, "availableCity")->valuestring;
    const CostMatrix::CityIdxType avl_loc = _cost_prob.city_idx(avl_city_name);
    const double avl_time_stamp_ms =
      cJSON_GetObjectItem(json_vehicle, "earliestAvailableTime")->valuedouble;
    assert(avl_time_stamp_ms > 0.0);
    const double avl_time = _ms2hours(avl_time_stamp_ms);
    _sorted_vehicles[i] = new Vehicle(name, avl_loc, avl_time);
  }
  cJSON_Delete(json);  // TODO: keep the json and reuse its const strings
  return 0;
}

int
Scheduler::_init_order_tasks_from_json(const char *filename)
{
  _num_orders = 0;
  return 0;
}

