#include "scheduler.h"
#include "route.h"
#include "cost.h"
#include "task.h"
#include "step.h"
#include "vehicle.h"
#include "fileio.h"
#include "timer.h"
#include "membuf.h"
#include <omp.h>
#include <set>
#include <vector>
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
    assert(ms >= 0.0);
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
  : _mb(0), _num_sorted_vehicles(0), _num_sorted_orders(0),
  _cost_prob(cst_prb), _categorized_orders(0)
{
  if (!_init_vehicles_from_json(vehicle_file))
    _init_order_tasks_from_json(order_file);
}

Scheduler::~Scheduler()
{
  double t1 = get_wall_time();
  const size_t ncities = _cost_prob.num_cities();
  for (int fri = 0; fri < ncities; ++fri)
  {
    for (int toi = 0; toi < ncities; ++toi)
      delete []_categorized_orders[fri][toi];
    delete []_categorized_orders[fri];
  }
  delete []_categorized_orders;
  _categorized_orders = 0;
  print_wall_time_diff(t1, "Destruct categorizied orders matrix");
  t1 = get_wall_time();
  // #pragma omp parallel for
  for (vector<Vehicle *>::size_type i = 0; i < _sorted_vehicles.size(); ++i)
    delete _sorted_vehicles[i];
  print_wall_time_diff(t1, "Destruct vehicles");
  t1 = get_wall_time();
  // #pragma omp parallel for
  // for (vector<OrderTask *>::size_type i = 0; i < _orders.size(); ++i)
  //   if (_orders[i])
  //     _orders[i]->~OrderTask();
  delete[] _sorted_orders;
  _sorted_orders = 0;
  print_wall_time_diff(t1, "Destruct OrderTasks");
  t1 = get_wall_time();
  delete _mb;
  _mb = 0;
  print_wall_time_diff(t1, "Destruct MemBufs");
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
    assert(avl_time_stamp_ms >= 0.0);
    const double avl_time = _ms2hours(avl_time_stamp_ms);
    _sorted_vehicles.push_back(new Vehicle(name, avl_loc, avl_time));
  }
  cJSON_Delete(json);  // TODO: keep the json and reuse its const strings
  print_wall_time_diff(t1, "Create vehicles with JSON objects");
  t1 = get_wall_time();
  stable_sort(_sorted_vehicles.begin(), _sorted_vehicles.end(), Vehicle::cmp);
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
  _mb = new SchedulerMemBuf(num_orig_orders);
  print_wall_time_diff(t1, "Create Scheduler MemBuf");
  t1 = get_wall_time();
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
    vsp_debug && cout << "~~ from_city->valuestring = " << from_city->valuestring
      << " to_city->valuestring = " << to_city->valuestring
      << " pickup_time->valuedouble = " << pickup_time->valuedouble
      << " expected_start_time = " << expected_start_time
      << " prob = " << prob
      << endl;
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
    const int thread_id = omp_get_thread_num();
    orders[i] = new (_mb->order_task[thread_id]->allocate())
      OrderTask(from_loc, to_loc, expected_start_time, prob, is_virtual,
                name, receivable, load_time, unload_time, line_expense);
    vsp_debug && cout << "~~ from_loc = " << from_loc
      << " to_loc = " << to_loc
      << " pickup_time->valuedouble = " << pickup_time->valuedouble
      << " expected_start_time = " << expected_start_time
      << " prob = " << prob
      << endl;
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
  _make_categorized_orders();
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
  stable_sort(sorted_orders.begin(), sorted_orders.end(), Task::cmp);
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
Scheduler::_make_categorized_orders()
{
  double t1 = get_wall_time();
  // allocate memory for categorized orders matrix
  const size_t ncities = _cost_prob.num_cities();
  const size_t nticks = CostMatrix::num_hour_ticks();
  _categorized_orders = new std::vector<OrderTask *> **[ncities];
  for (size_t fri = 0; fri < ncities; ++fri)
  {
    _categorized_orders[fri] = new std::vector<OrderTask *> *[ncities];
    for (size_t toi = 0; toi < ncities; ++toi)
      _categorized_orders[fri][toi] = new std::vector<OrderTask *> [nticks];
  }
  // categorize orders
  assert(_sorted_orders || !"_sorted_orders must be made before!");
  for (size_t i = 0; i < _num_sorted_orders; ++i)
  {
    OrderTask *pot = _sorted_orders[i];
    const size_t fri = pot->loc_from(), toi = pot->loc_to();
    const size_t tki = CostMatrix::hour_tick(pot->expected_start_time());
    _categorized_orders[fri][toi][tki].push_back(pot);
  }
  print_wall_time_diff(t1, "Create categorized orders matrix");
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
  #pragma omp parallel for schedule(dynamic)
  for (size_t i = 0; i < _num_sorted_orders; ++i)
  {
    OrderTask *order = _sorted_orders[i];
    for (size_t j = i + 1; j < _num_sorted_orders; ++j)
    {
      OrderTask *next_candidate = _sorted_orders[j];
      if (order->connect(*next_candidate, _cost_prob, _mb, omp_get_thread_num(),
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

namespace
{
  inline bool _isTerminalRoute(const Route *route)
  {
    assert(route);
    return route->next_steps().empty();
  }

  inline bool _isTerminalTask(const OrderTask *task)
  {
    assert(task);
    return _isTerminalRoute(task->max_net_value_route());
  }
}

void
Scheduler::_update_terminal_net_value()
{
  const size_t ncities = _cost_prob.num_cities();
  const size_t nticks = CostMatrix::num_hour_ticks();
  for (size_t fri = 0; fri < ncities; ++fri)
    for (size_t toi = 0; toi < ncities; ++toi)
      for (size_t tki = 0; tki < nticks; ++tki)
      {
        vector<OrderTask *> &tasks = _categorized_orders[fri][toi][tki];
        if (tasks.empty())
          continue;
        OrderTask *task_1st =
          *min_element(tasks.begin(), tasks.end(), Task::cmp);
        Route *route_1st = task_1st->max_net_value_route();
        if (!route_1st)
          continue;
        for (vector<OrderTask *>::iterator it = tasks.begin();
             it != tasks.end(); ++it)
        {
          OrderTask *task = *it;
          assert(task);
          if (!_isTerminalTask(task))
            continue;
#if 0
          const double TIME_WINDOW_IN_HOUR = 240.0;
          const double new_net_value =
            route_1st->net_value() / TIME_WINDOW_IN_HOUR
            * task->max_net_value_route()->duration();
#else
          const double new_net_value = route_1st->net_value();
#endif
          task->max_net_value_route()->net_value(new_net_value);
        }
      }
}

void
Scheduler::_reset_order_net_value_stuff()
{
  assert(_sorted_orders);
  for (size_t i = 0; i < _num_sorted_orders; ++i)
  {
    _sorted_orders[i]->reset_max_net_value_route();
    vector<Route *> &routes = _sorted_orders[i]->routes();
    for (vector<Route *>::iterator it = routes.begin();
         it != routes.end(); ++it)
      if (!_isTerminalRoute(*it))
        (*it)->reset_net_value_stuff();
  }
}

extern int NITER;

void
Scheduler::run()
{
  _analyze_orders();
  double t1 = get_wall_time();
  for (vector<Vehicle *>::iterator it = _sorted_vehicles.begin();
       it != _sorted_vehicles.end(); ++it)
    for (int i = 0;;)
    {
      Vehicle *vehicle = *it;
      vehicle->compute_net_value();
      if (++i >= NITER)
        break;
      else
        cout << "\r[#] Iteration " << i << "          " << flush;
      // prepare for next iteration
      _update_terminal_net_value();
      _reset_order_net_value_stuff();
      vehicle->reset_net_value_stuff();
    }
  cout << endl;
  print_wall_time_diff(t1, "Compute net values of vehicles");
#ifdef DEBUG
  Route::print_num_objs();
  Step::print_num_objs();
  EmptyRunTask::print_num_objs();
  OrderTask::print_num_objs();
#endif
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

void
Scheduler::dump_value_networks(const char *filename)
{
  double t1 = get_wall_time();
  cJSON *json = cJSON_CreateObject();
  for (vector<Vehicle *>::iterator it = _sorted_vehicles.begin();
       it != _sorted_vehicles.end(); ++it)
    cJSON_AddItemToObject(json, (*it)->name().c_str(),
                          (*it)->to_treemap(_cost_prob));
  print_wall_time_diff(t1, "Create a value network for each vehicle");
  t1 = get_wall_time();
  char *json_str = cJSON_PrintBuffered(json, 10000000, 1);  // TODO: nonformated
  cJSON_Delete(json);
  print_wall_time_diff(t1, "Convert JSON object to char *");
  t1 = get_wall_time();
  ofstream outf(filename);
  outf << json_str;
  outf.close();
  free(json_str);
  print_wall_time_diff(t1, "Print JSON to file and free char *");
}

