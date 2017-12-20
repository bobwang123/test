#ifndef __COST_H__
#define __COST_H__

#include "cJSON.h"
#include <vector>
#include <string>
#include <map>

class Cost
{
  double _distance;
  double _expense;
  double _duration;
public:
  Cost(const double distance=0.0,
       const double expense =0.0,
       const double duration=0.0)
    : _distance(distance), _expense(expense), _duration(duration)
  {}
  Cost(const Cost &cst)
    : _distance(cst._distance), _expense(cst._expense), _duration(cst._duration)
  {}
  ~Cost()
  {}
public:
  const double distance() const { return _distance; }
  const double expense()  const { return _expense;  }
  const double duration() const { return _duration; }
  Cost &operator=(const Cost &cst)
  {
    if (this != &cst)
    {
      _distance = cst._distance;
      _expense  = cst._expense;
      _duration = cst._duration;
    }
    return *this;
  }
};

class CostMatrix
{
  // each hour each distribution per day
  static const std::size_t _NUM_PROB_TICKS = 24;
  typedef std::map<std::string, Cost> _BaseCostMapType;
  typedef double _BaseProbArrayType[_NUM_PROB_TICKS];
public:
  typedef std::vector<std::string>::size_type CityIdxType;
private:
  // [CityIdxType] -> city_name
  std::vector<std::string> _cities;
  // [city_name] -> CityIdxType
  std::map<std::string, CityIdxType> _city_indices;
  // [from][to][route_name] -> Cost
  _BaseCostMapType **_cost_mat;
  // [from][to][hour_tick] -> double
  _BaseProbArrayType **_prob_mat;
public:
  explicit CostMatrix(const char *filename);
  ~CostMatrix();
private:
  std::vector<std::string> &_create_cities(cJSON *json);
  std::map<std::string, CityIdxType> &_create_city_indices(cJSON *json);
  _BaseCostMapType **_create_cost_mat(cJSON *json);
  _BaseProbArrayType **_create_prob_mat(cJSON *json);
  int _parse_cost_json(cJSON *json);
};

#endif /* __COST_H__ */
