#include <membuf.h>
#include <iostream>

using namespace std;

struct A
{
  const int n;
  A(const int num): n(num)
  {
    cout << "construct A: " << n << endl;
  }
  ~A()
  {
    cout << "destruct A: " << n << endl;
  }
};

int main()
{
  const int n = 100;
  MemBuf<A> mb(0);
  for (int i = 0; i < n; ++i)
  {
    const A *p = new (mb.allocate()) A(i);
    cout << "placement new obj: " << p->n << endl;
  }
  cout << "Unit Test" << endl;
  return 0;
}
