#ifndef __MEMBUF_H__
#define __MEMBUF_H__

#include "route.h"
#include "task.h"
#include "step.h"
#include <omp.h>
#include <iostream>
#include <vector>

// Lock-free memory buffer
template <typename ObjType>
class MemBuf
{
  std::vector<char *>_buf;
  const size_t _init_capacity;
  size_t _capacity;
  size_t _size;
  const size_t _obj_size;
  char *_cursor;
public:
  MemBuf(const size_t n):
    _buf(), _init_capacity(n > 32 ? n : 32), _capacity(_init_capacity),
    _size(0), _obj_size(sizeof(ObjType)), _cursor(0)
  {
    _buf.push_back(new char[_obj_size * _capacity]);
    _cursor = _buf.back();
#ifdef DEBUG
    std::cout << "DEBUG - Construct MemBuf " << this << std::endl;
#endif
  }
  ~MemBuf()
  {
    for(std::vector<char *>::iterator it = _buf.begin(); it != _buf.end(); ++it)
      delete [](*it);
    _capacity = 0;
    _size = 0;
#ifdef DEBUG
    std::cout << "DEBUG - Destruct MemBuf " << this << std::endl;
#endif
  }
public:
  // allocate memory for one object: placement new
  char *allocate()
  {
    ++_size;
    char *old_cursor = 0;
    if (_size <= _capacity)
    {
      old_cursor = _cursor;
    }
    else // enlarge the buffer
    {
      _buf.push_back(new char[_obj_size * _capacity]);
      _capacity += _capacity;
      old_cursor = _cursor = _buf.back();
      std::cout << "enlarge MemBuf to " << _capacity << std::endl;
    }
    _cursor += _obj_size;
    return old_cursor;
  }
};

class SchedulerMemBuf
{
public:
  const size_t num_threads;
  std::vector<MemBuf<Route> *> route;
  std::vector<MemBuf<Step> *> step;
  std::vector<MemBuf<EmptyRunTask> *> empty_run_task;
  std::vector<MemBuf<OrderTask> *> order_task;
public:
  SchedulerMemBuf(const size_t num_orders):
    num_threads(omp_get_max_threads()),
    route(num_threads),
    step(num_threads),
    empty_run_task(num_threads),
    order_task(num_threads)
  {
    const size_t num_orders_per_thread = num_orders / num_threads;
    // This size must be enough theoretically
    const size_t init_buf_size_th = (2*num_orders - 2 - num_orders_per_thread)
      * (num_orders_per_thread - 1) / 2;
    // Usually the memory requirement is no greater than 1/10 of theory.
    // Choose 1/8
    const size_t shrink_factor = 8;
    // Effective init size
    const size_t init_buf_size_eff = init_buf_size_th / shrink_factor;
    for (size_t i = 0; i < num_threads; ++i)
    {
      route[i] = new MemBuf<Route>(init_buf_size_eff);
      step[i] = new MemBuf<Step>(init_buf_size_eff);
      empty_run_task[i] = new MemBuf<EmptyRunTask>(init_buf_size_eff);
      order_task[i] = new MemBuf<OrderTask>(num_orders_per_thread);
    }
  }
  ~SchedulerMemBuf()
  {
    for (size_t i = 0; i < num_threads; ++i)
    {
      delete route[i];
      delete step[i];
      delete empty_run_task[i];
      delete order_task[i];
    }
  }
};

#endif  // __MEMBUF_H__
