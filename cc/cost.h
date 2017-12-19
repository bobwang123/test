#ifndef __COST_H__
#define __COST_H__

#include <vector>
#include <string>
#include <map>

class Cost
{
  const double _distance;
  const double _expense;
  const double _duration;
public:
  Cost(const double distance=0.0,
       const double expense=0.0,
       const double duration=0.0)
    :_distance(distance), _expense(expense), _duration(duration) {}
  ~Cost() {}
public:
  const double distance() const { return _distance; }
  const double expense()  const { return _expense;  }
  const double duration() const { return _duration; }
};

class CostMatrix
{
  std::vector<std::string> _cities;
  std::map<std::string, int> _city_indices;
  const Cost **_cost_mat;
  const Cost **_prob_mat;
  static const int _num_prob_ticks = 24;  // each hour each distribution per day
public:
  explicit CostMatrix(const char *filename);
  ~CostMatrix();
};

#endif /* __COST_H__ */
