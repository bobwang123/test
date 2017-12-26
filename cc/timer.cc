#include "timer.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sys/time.h>

using namespace std;

double get_wall_time()
{
  struct timeval time;
  if (gettimeofday(&time,NULL))
      return 0; //  Handle error
  return (double)time.tv_sec + (double)time.tv_usec * .000001;
}

void print_wall_time_diff(const double start, const char *msg)
{
  const double now = get_wall_time();
  cout << "Wall time - " << msg << " : "
    << std::fixed << std::setprecision(4) << now - start
    << " s" << endl;
}

double get_cpu_time()
{
    return (double)clock() / CLOCKS_PER_SEC;
}

