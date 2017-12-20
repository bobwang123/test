#include "cost.h"

#ifdef DEBUG
extern const int vsp_debug = 1;
#else
extern const int vsp_debug = 0;
#endif  // DEBUG

int main()
{
  CostMatrix cm("cost_prob.cc.json");
  return 0;
}
