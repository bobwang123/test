#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <vector>

class OrderTask;
class Vehicle;
class CostMatrix;

class Scheduler
{
  std::vector<Vehicle *>_sorted_vehicles;  // sorted by avl_time
  std::size_t _num_sorted_vehicles;
  std::vector<OrderTask *>_orders;  // all orders
  OrderTask **_sorted_orders;  // filtered orders sorted by expected_start_time
  std::size_t _num_sorted_orders;
  const CostMatrix &_cost_prob;
  void *_opt;
  static const double _PROB_TH;  // probability threshold to filter orders
  static const double _DEFAULT_LOAD_TIME;  // hours
  static const double _DEFAULT_UNLOAD_TIME;  // hours
public:
  Scheduler(const char *order_file,
            const char *vehicle_file,
            const CostMatrix &cst_prb,
            void *opt);
  ~Scheduler();
private:
  int
    _init_vehicles_from_json(const char *filename);
  int
    _init_order_tasks_from_json(const char *filename);
  void
    _ignore_unreachable_orders_and_sort();
  void
    _build_order_dag();
  void
    _analyze_orders();
public:
  void
    run();
  void
    dump_plans(const char *filename);
};

#endif  // __SCHEDULER_H__

