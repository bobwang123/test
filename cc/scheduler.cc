#include "scheduler.h"
#include "route.h"
#include "cost.h"
#include "task.h"
#include "vehicle.h"
#include "fileio.h"
#include <set>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <cassert>

using namespace std;

extern const int vsp_debug;

namespace
{
  // convert milliseconds to hours since 1970-1-1 00:00:00
  double _ms2hours(const double ms)
  {
    assert(ms > 0.0);
    return ms / 3600e3;
  }
}

// ignore small probability predicted(virtual) orders
const double
Scheduler::_PROB_TH = 0.05;

const double
Scheduler::_DEFAULT_LOAD_TIME = 5;

const double
Scheduler::_DEFAULT_UNLOAD_TIME = 5;

const double
Scheduler::_DEFAULT_MAX_WAIT_TIME = 72;

const double
Scheduler::_DEFAULT_MAX_MAX_EMPTY_DIST = 500;

Scheduler::Scheduler(const char *vehicle_file,
                     const char *order_file,
                     const CostMatrix &cst_prb,
                     void *opt)
  :_sorted_vehicles(0),
  _num_sorted_vehicles(0),
  _sorted_orders(0),
  _num_sorted_orders(0),
  _cost_prob(cst_prb),
  _opt(opt)
{
  if (!_init_vehicles_from_json(vehicle_file))
    _init_order_tasks_from_json(order_file);
}

Scheduler::~Scheduler()
{
  for (vector<Vehicle *>::iterator it = _sorted_vehicles.begin();
       it != _sorted_vehicles.end(); ++it)
    delete *it;
  for (std::vector<OrderTask *>::iterator it = _orders.begin();
       it != _orders.end(); ++it)
    delete *it;
  delete[] _sorted_orders;
  _sorted_orders = 0;
}

int
Scheduler::_init_vehicles_from_json(const char *filename)
{
  cJSON *json = parse_json_file(filename);
  cJSON *json_array_vehicles = cJSON_GetObjectItem(json, "data");
  _num_sorted_vehicles = cJSON_GetArraySize(json_array_vehicles);
  _sorted_vehicles.reserve(_num_sorted_vehicles);
  for (int i = 0; i < _num_sorted_vehicles; ++i)
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
    _sorted_vehicles.push_back(new Vehicle(name, avl_loc, avl_time));
  }
  cJSON_Delete(json);  // TODO: keep the json and reuse its const strings
  sort(_sorted_vehicles.begin(), _sorted_vehicles.end(), Vehicle::reverse_cmp);
  return 0;
}

int
Scheduler::_init_order_tasks_from_json(const char *filename)
{
  cJSON *json = parse_json_file(filename);
  cJSON *json_array_orders = cJSON_GetObjectItem(json, "data");
  const int num_orders = cJSON_GetArraySize(json_array_orders);
  _orders.reserve(num_orders);
  bool has_real = false;
  // TODO: a change to improve using multi-thread
  for (int i = 0; i < num_orders; ++i)
  {
    cJSON *json_order = cJSON_GetArrayItem(json_array_orders, i);
    cJSON *from_city = cJSON_GetObjectItem(json_order, "fromCity");
    const CostMatrix::CityIdxType from_loc =
      _cost_prob.city_idx(from_city->valuestring);
    cJSON *to_city = cJSON_GetObjectItem(json_order, "toCity");
    const CostMatrix::CityIdxType to_loc =
      _cost_prob.city_idx(to_city->valuestring);
    cJSON *pickup_time = cJSON_GetObjectItem(json_order, "orderedPickupTime");
    const double expected_start_time = _ms2hours(pickup_time->valuedouble);
    cJSON *order_is_virtual = cJSON_GetObjectItem(json_order, "isVirtual");
    const bool is_virtual = !!(order_is_virtual? order_is_virtual->valueint: 0);
    if (!is_virtual)
    {
      has_real = true;
      vsp_debug && cout << "Found real order: " << from_city->valuestring << "->"
        << to_city->valuestring << "\n";
    }
    const double prob =
      is_virtual? _cost_prob.prob(from_loc, to_loc, expected_start_time): 1.0;
    if (is_virtual && prob < _PROB_TH)
      continue;  // ignore this virtual order due to too small probability
    cJSON *order_id = cJSON_GetObjectItem(json_order, "orderId");
    const char *name = order_id->valuestring;
    cJSON *order_receivable = cJSON_GetObjectItem(json_order, "orderMoney");
    const double receivable = order_receivable->valuedouble;
    cJSON *order_load_time = cJSON_GetObjectItem(json_order, "loadingTime");
    const double load_time = order_load_time->type == cJSON_NULL
      ? _DEFAULT_LOAD_TIME: order_load_time->valuedouble;
    cJSON *order_unload_time = cJSON_GetObjectItem(json_order, "unloadingTime");
    const double unload_time = order_unload_time->type == cJSON_NULL
      ? _DEFAULT_UNLOAD_TIME: order_unload_time->valuedouble;
    _orders.push_back(new OrderTask(from_loc, to_loc, expected_start_time,
                                    prob, is_virtual, name, receivable,
                                    load_time, unload_time));
  }
  cJSON_Delete(json);  // TODO: keep the json and reuse its const strings
  if (!has_real)
    cout << "** Warning: No real orders found! There are only virtual orders.\n";
  assert(_orders.size() <= num_orders);
  cout << "Ignore " << num_orders - _orders.size()
    << " virtual (predicted) orders in total " << num_orders
    << " orders due to probability <" << _PROB_TH * 100 << "%\n";
  _ignore_unreachable_orders_and_sort();
  return 0;
}

void
Scheduler::_ignore_unreachable_orders_and_sort()
{
  set<OrderTask *> all_reachable_orders;
  for (int i = 0; i < _num_sorted_vehicles; ++i)
    for (vector<OrderTask *>::iterator it = _orders.begin();
         it != _orders.end(); ++it)
      if (_sorted_vehicles[i]->connect(*(*it), _cost_prob))
        all_reachable_orders.insert(*it);
  _num_sorted_orders = all_reachable_orders.size();
  cout << "Ignore " << _orders.size() - _num_sorted_orders
    << " unreachable orders in total " << _orders.size()
    << " large probability orders.\n"
    << _num_sorted_orders
    << " reachable large probability orders to be scheduled.\n";
  vector<OrderTask *> sorted_orders(all_reachable_orders.begin(),
                                    all_reachable_orders.end());
  if (vsp_debug)
  {
    cout << "DEBUG - Unsorted Order Expected Start Time\n";
    for (vector<OrderTask *>::iterator it = sorted_orders.begin();
         it != sorted_orders.end(); ++it)
      cout << (*it)->expected_start_time() << "\n";
  }
  sort(sorted_orders.begin(), sorted_orders.end(), Task::reverse_cmp);
  // raw array should be faster than vector when randomly accessing
  _sorted_orders = new OrderTask *[_num_sorted_orders];
  for (vector<OrderTask *>::size_type i = 0; i < _num_sorted_orders; ++i)
    _sorted_orders[i] = sorted_orders[i];
  if (vsp_debug)
  {
    cout << "DEBUG - Sorted Order Expected Start Time\n";
    for (int i = 0; i < _num_sorted_orders; ++i)
      cout << _sorted_orders[i]->expected_start_time() << "\n";
  }
}

void
Scheduler::_build_order_cost()
{
  // TODO: outer loop can be parallel
  for (int i = 0; i < _num_sorted_orders; ++i)
  {
    OrderTask *order = _sorted_orders[i];
    const CostMatrix::CostMapType &costs =
      _cost_prob.costs(order->loc_from(), order->loc_to());
    for (CostMatrix::CostMapType::const_iterator cit = costs.begin();
         cit != costs.end(); ++cit)
      order->add_route(new Route(*order, cit->first, cit->second));
  }
}

void
Scheduler::_build_order_dag()
{
  size_t num_edges = 0;
  // TODO: outer loop can be parallel
  for (int i = 0; i < _num_sorted_orders; ++i)
  {
    OrderTask *order = _sorted_orders[i];
    for (int j = i + 1; j < _num_sorted_orders; ++j)
    {
      OrderTask *next_candidate = _sorted_orders[j];
      if (order->connect(*next_candidate, _cost_prob,
                         _DEFAULT_MAX_WAIT_TIME, _DEFAULT_MAX_MAX_EMPTY_DIST))
        ++num_edges;
    }
  }
}

void
Scheduler::_analyze_orders()
{
  _build_order_cost();
  _build_order_dag();
}

void
Scheduler::run()
{
  _analyze_orders();
  for (vector<Vehicle *>::iterator it = _sorted_vehicles.begin();
       it != _sorted_vehicles.end(); ++it)
    (*it)->compute_max_profit();
}

void
Scheduler::dump_plans(const char *filename)
{
  cJSON *all_plans_sorted = cJSON_CreateArray();
  for (vector<Vehicle *>::iterator it = _sorted_vehicles.begin();
       it != _sorted_vehicles.end(); ++it)
  {
    Vehicle *v = *it;
    v->sorted_candidate_plans();
    cJSON_AddItemToArray(all_plans_sorted, v->plans_to_dict(_cost_prob));
  }
  cJSON *json = cJSON_CreateObject();
  cJSON_AddItemToObjectCS(json, "data", all_plans_sorted);
  char *json_str = cJSON_PrintBuffered(json, 10000000, 1);
  cJSON_Delete(json);
  ofstream outf(filename);
  outf << json_str;
  outf.close();
  free(json_str);
}

