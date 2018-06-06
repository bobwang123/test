#ifndef __PLAN_H__
#define __PLAN_H__

#include "step.h"
#include "cost.h"
#include "cJSON.h"
#include <vector>

// A plan is composed of consecutive steps
class Plan
{
  std::vector<const Step *>_steps;
  cJSON *
    _steps_to_cJSON_Array(const CostMatrix &cost_prob_mat) const
  {
    cJSON *step_arr = cJSON_CreateArray();
    for (std::vector<const Step *>::const_iterator cit = _steps.begin();
         cit != _steps.end(); ++cit)
    {
      cJSON *empty_and_order = (*cit)->to_dict(cost_prob_mat);
      cJSON *empty = cJSON_DetachItemFromArray(empty_and_order, 0);
      cJSON *order = cJSON_DetachItemFromArray(empty_and_order, 0);
      cJSON_Delete(empty_and_order);  // make sure no memory leak
      cJSON_AddItemToArray(step_arr, empty);
      cJSON_AddItemToArray(step_arr, order);
    }
    return step_arr;
  }
  static const std::size_t _NUM_RESERVED_STEPS = 16;
public:
  Plan() { _steps.reserve(_NUM_RESERVED_STEPS); }
  ~Plan() {}
public:
  void
    append(const Step *s) { _steps.push_back(s); }
  const double
    expected_net_value() const
    {
      if (_steps.empty())
        return Consts::DOUBLE_NONE;
      const Step *first_step = _steps.front();
      return first_step->net_value() * first_step->prob();
    }
  const double
    total_gross_margin() const
    {
      if (_steps.empty())
        return Consts::DOUBLE_NONE;
      double sum_gross_margin = 0.0;
      for (std::vector<const Step *>::const_iterator cit = _steps.begin();
           cit != _steps.end(); ++cit)
        sum_gross_margin += (*cit)->gross_margin();
      return sum_gross_margin;
    }
  const double
    joint_prob() const
    {
      if (_steps.empty())
        return Consts::DOUBLE_NONE;
      double total_prob = 1.0;
      for (std::vector<const Step *>::const_iterator cit = _steps.begin();
           cit != _steps.end(); ++cit)
        total_prob *= (*cit)->prob();
      return total_prob;
    }
  cJSON *
    to_dict(const CostMatrix &cost_prob_mat) const
    {
        cJSON *plan_dict = cJSON_CreateObject();
        cJSON_AddItemToObjectCS(plan_dict, "expectedNetValue",
                                cJSON_CreateNumber(expected_net_value()));
        cJSON_AddItemToObjectCS(plan_dict, "totalGrossMargin",
                                cJSON_CreateNumber(total_gross_margin()));
        cJSON_AddItemToObjectCS(plan_dict, "jointProbability",
                                cJSON_CreateNumber(joint_prob()));
        cJSON_AddItemToObjectCS(plan_dict, "routes",
                                _steps_to_cJSON_Array(cost_prob_mat));
        return plan_dict;
    }
};

#endif  // __PLAN_H__
