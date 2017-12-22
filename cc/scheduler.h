#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <vector>

class OrderTask;
class Vehicle;
class CostMatrix;

class Scheduler
{
  Vehicle *_sorted_vehicles;  // sorted by avl_time
  OrderTask *_sorted_orders;  // sorted by expected_start_time
  const CostMatrix &_cost_prob;
  void *_opt;
public:
  Scheduler(const char *order_file, const char *vehicle_file,
            const CostMatrix &cst_prb, void *opt);
  ~Scheduler();
private:
  int _init_vehicles_from_json(const char *filename);
  int _init_order_tasks_from_json(const char *filename);
};

#endif  // __SCHEDULER_H__

