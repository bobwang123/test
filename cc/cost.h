#ifndef __COST_H__
#define __COST_H__

#include "cJSON.h"
#include <vector>
#include <string>
#include <map>
#include <cmath>
#include <iostream>

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
  typedef double _BaseProbArrayType[_NUM_PROB_TICKS];
public:
  typedef std::vector<std::string>::size_type CityIdxType;
  typedef std::map<std::string, Cost> CostMapType;
private:
  // [CityIdxType] -> city_name
  std::vector<std::string> _cities;
  // [city_name] -> CityIdxType
  std::map<std::string, CityIdxType> _city_indices;
  // [from][to] -> [route_name]->Cost
  CostMapType **_cost_mat;
  // [from][to][hour_tick] -> double
  _BaseProbArrayType **_prob_mat;
public:
  explicit CostMatrix(const char *filename);
  ~CostMatrix();
  static std::size_t hour_tick(double timestamp_in_hour)
  {
    const std::size_t time_zone = +8;
    return static_cast<std::size_t>(std::floor(timestamp_in_hour + time_zone))
      % _NUM_PROB_TICKS;
  }
private:
  std::vector<std::string> &
    _create_cities(cJSON *json);
  std::map<std::string, CityIdxType> &
    _create_city_indices(cJSON *json);
  CostMapType **
    _create_cost_mat(cJSON *json);
  _BaseProbArrayType **
    _create_prob_mat(cJSON *json);
  int
    _parse_cost_json(cJSON *json);
public:
  CityIdxType
    city_idx(const std::string &city_name) const
    { return _city_indices.at(city_name); }
  const std::string &
    city_name(const CityIdxType idx) const
    { return _cities.at(idx); }
  double
    prob(CityIdxType start_loc,
         CityIdxType end_loc,
         double time_in_hour) const
    {
      return _prob_mat[start_loc][end_loc][hour_tick(time_in_hour)];
    }
  const CostMapType &
    costs(CityIdxType start_loc,
          CityIdxType end_loc) const
    { return _cost_mat[start_loc][end_loc]; }
  std::size_t
    num_cities() const { return _cities.size(); }
  static std::size_t
    num_hour_ticks() { return _NUM_PROB_TICKS; }
};

#endif /* __COST_H__ */
