#ifndef __TASK_H__
#define __TASK_H__

class Task
{
public:
  double receivable() const { return 0.0; }
  double prob() const { return 0.0; }
  double expected_start_time() const { return 0.0; }
  double no_run_time() const { return 0.0; }
  bool is_virtual() const { return true; }
};

#endif /* __TASK_H__ */
