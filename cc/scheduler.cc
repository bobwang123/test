#include "scheduler.h"
#include "route.h"
#include "cost.h"
#include "task.h"
#include "vehicle.h"
#include "fileio.h"
#include "timer.h"
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

// drop small probability predicted(virtual) orders
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
                     const CostMatrix &cst_prb)
  : _num_sorted_vehicles(0), _num_sorted_orders(0), _cost_prob(cst_prb)
{
  if (!_init_vehicles_from_json(vehicle_file))
    _init_order_tasks_from_json(order_file);
}

Scheduler::~Scheduler()
{
  double t1 = get_wall_time();
  // #pragma omp parallel for
  for (vector<Vehicle *>::size_type i = 0; i < _sorted_vehicles.size(); ++i)
    delete _sorted_vehicles[i];
  print_wall_time_diff(t1, "Destruct vehicles");
  t1 = get_wall_time();
  // #pragma omp parallel for
  for (vector<OrderTask *>::size_type i = 0; i < _orders.size(); ++i)
    delete _orders[i];
  delete[] _sorted_orders;
  _sorted_orders = 0;
  print_wall_time_diff(t1, "Destruct OrderTasks");
}

int
Scheduler::_init_vehicles_from_json(const char *filename)
{
  double t1 = get_wall_time();
  cJSON *json = parse_json_file(filename);
  print_wall_time_diff(t1, "Parse vehicle JSON file");
  t1 = get_wall_time();
  cJSON *json_array_vehicles = cJSON_GetObjectItem(json, "data");
  _num_sorted_vehicles = cJSON_GetArraySize(json_array_vehicles);
  _sorted_vehicles.reserve(_num_sorted_vehicles);
  for (size_t i = 0; i < _num_sorted_vehicles; ++i)
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
  print_wall_time_diff(t1, "Create vehicles with JSON objects");
  t1 = get_wall_time();
  sort(_sorted_vehicles.begin(), _sorted_vehicles.end(), Vehicle::cmp);
  print_wall_time_diff(t1, "Sort vehicles with avl_time");
  return 0;
}

int
Scheduler::_init_order_tasks_from_json(const char *filename)
{
  double t1 = get_wall_time();
  cJSON *json = parse_json_file(filename);
  print_wall_time_diff(t1, "Parse order JSON file");
  t1 = get_wall_time();
  cJSON *json_array_orders = cJSON_GetObjectItem(json, "data");
  const int num_orig_orders = cJSON_GetArraySize(json_array_orders);
  vector<OrderTask *> orders(num_orig_orders, static_cast<OrderTask*>(0));
  #pragma omp parallel for
  for (int i = 0; i < num_orig_orders; ++i)
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
      vsp_debug && cout << "Found real order: " << from_city->valuestring << "->"
        << to_city->valuestring << endl;
    const double prob =
      is_virtual? _cost_prob.prob(from_loc, to_loc, expected_start_time): 1.0;
    if (is_virtual && prob < _PROB_TH)
      continue;  // drop this virtual order due to too small probability
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
    cJSON *order_line_expense = cJSON_GetObjectItem(json_order, "lineCost");
    const double line_expense =
      !order_line_expense || order_line_expense->type == cJSON_NULL
      ? Consts::DOUBLE_NONE: order_line_expense->valuedouble;
    orders[i] = new OrderTask(from_loc, to_loc, expected_start_time,
                              prob, is_virtual, name, receivable,
                              load_time, unload_time, line_expense);
  }
  cJSON_Delete(json);  // TODO: keep the json and reuse its const strings
  const size_t num_orders = num_orig_orders -
    count(orders.begin(), orders.end(), static_cast<OrderTask*>(0));
  cout << "Drop " << num_orig_orders - num_orders
    << " virtual (predicted) orders in total " << num_orig_orders
    << " orders due to probability < " << _PROB_TH * 100 << "%" << endl;
  print_wall_time_diff(t1, "Create orders with JSON objects");
  _orders = orders;
  _ignore_unreachable_orders_and_sort(num_orders);
  return 0;
}

namespace
{
  inline bool _is_real(OrderTask *t) { return t && !t->is_virtual(); }
}

void
Scheduler::_ignore_unreachable_orders_and_sort(const size_t num_orders)
{
  double t1 = get_wall_time();
  set<OrderTask *> all_reachable_orders;
  for (size_t i = 0; i < _num_sorted_vehicles; ++i)
    for (vector<OrderTask *>::iterator it = _orders.begin();
         it != _orders.end(); ++it)
      if (*it && _sorted_vehicles[i]->connect(*(*it), _cost_prob))
        all_reachable_orders.insert(*it);
  _num_sorted_orders = all_reachable_orders.size();
  assert(_num_sorted_orders <= num_orders);
  cout << "Drop " << num_orders - _num_sorted_orders
    << " unreachable orders in total " << num_orders
    << " large probability orders.\n";
  print_wall_time_diff(t1, "Check and drop unreachable orders");
  t1 = get_wall_time();
  const int num_real_orders =
    count_if(_orders.begin(), _orders.end(), _is_real);
  if (num_real_orders)
    cout << num_real_orders << " real orders found.\n";
  else
    cout << "** Warning: No real order found! There are only virtual orders.\n";
    cout << _num_sorted_orders
    << " reachable large probability orders to be scheduled." << endl;
  print_wall_time_diff(t1, "Count real orders");
  t1 = get_wall_time();
  vector<OrderTask *> sorted_orders(all_reachable_orders.begin(),
                                    all_reachable_orders.end());
  if (vsp_debug)
  {
    cout << "DEBUG - Unsorted Order Expected Start Time\n";
    for (vector<OrderTask *>::iterator it = sorted_orders.begin();
         it != sorted_orders.end(); ++it)
      cout << (*it)->expected_start_time() << "\n";
    cout << endl;
  }
  sort(sorted_orders.begin(), sorted_orders.end(), Task::cmp);
  // raw array should be faster than vector when randomly accessing
  _sorted_orders = new OrderTask *[_num_sorted_orders];
  for (vector<OrderTask *>::size_type i = 0; i < _num_sorted_orders; ++i)
    _sorted_orders[i] = sorted_orders[i];
  if (vsp_debug)
  {
    cout << "DEBUG - Sorted Order Expected Start Time\n";
    for (size_t i = 0; i < _num_sorted_orders; ++i)
      cout << _sorted_orders[i]->expected_start_time() << "\n";
    cout << endl;
  }
  print_wall_time_diff(t1, "Create sorted order array");
}

void
Scheduler::_build_order_cost()
{
  double t1 = get_wall_time();
  for (size_t i = 0; i < _num_sorted_orders; ++i)
  {
    OrderTask *order = _sorted_orders[i];
    const CostMatrix::CostMapType &costs =
      _cost_prob.costs(order->loc_from(), order->loc_to());
    for (CostMatrix::CostMapType::const_iterator cit = costs.begin();
         cit != costs.end(); ++cit)
      order->add_route(new Route(*order, cit->first, cit->second));
  }
  print_wall_time_diff(t1, "Init order costs");
}

void
Scheduler::_build_order_dag()
{
  double t1 = get_wall_time();
  vector<size_t> num_edges(_num_sorted_orders, 0);
  #pragma omp parallel for
  for (size_t i = 0; i < _num_sorted_orders; ++i)
  {
    OrderTask *order = _sorted_orders[i];
    for (size_t j = i + 1; j < _num_sorted_orders; ++j)
    {
      OrderTask *next_candidate = _sorted_orders[j];
      if (order->connect(*next_candidate, _cost_prob,
                         _DEFAULT_MAX_WAIT_TIME, _DEFAULT_MAX_MAX_EMPTY_DIST))
        ++num_edges[i];
    }
  }
  size_t total_num_edges = 0;
  #pragma omp parallel for reduction(+:total_num_edges)
  for (size_t i = 0; i < _num_sorted_orders; ++i)
    total_num_edges += num_edges[i];
  cout << "Create " << total_num_edges << " edges for order DAG.\n";
  print_wall_time_diff(t1, "Create order DAG");
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
  double t1 = get_wall_time();
  for (vector<Vehicle *>::iterator it = _sorted_vehicles.begin();
       it != _sorted_vehicles.end(); ++it)
    (*it)->compute_max_profit();
  print_wall_time_diff(t1, "Compute max profit of vehicles");
}

void
Scheduler::dump_plans(const char *filename)
{
  double t1 = get_wall_time();
  cJSON *all_plans_sorted = cJSON_CreateArray();
  for (vector<Vehicle *>::iterator it = _sorted_vehicles.begin();
       it != _sorted_vehicles.end(); ++it)
  {
    Vehicle *v = *it;
    v->sorted_candidate_plans();
    cJSON_AddItemToArray(all_plans_sorted, v->plans_to_dict(_cost_prob));
  }
  print_wall_time_diff(t1, "Sort and convert plans to JSON for vehicles");
  t1 = get_wall_time();
  cJSON *json = cJSON_CreateObject();
  cJSON_AddItemToObjectCS(json, "data", all_plans_sorted);
  char *json_str = cJSON_PrintBuffered(json, 10000000, 1);
  cJSON_Delete(json);
  print_wall_time_diff(t1, "Convert JSON object to char *");
  t1 = get_wall_time();
  ofstream outf(filename);
  outf << json_str;
  outf.close();
  free(json_str);
  print_wall_time_diff(t1, "Print JSON to file and free char *");
}

