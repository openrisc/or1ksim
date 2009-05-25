/* Simple test, that test function parameters */
#include "support.h"

int gk = 0;
int c = 0;
int fun1 (int a, int b, int c, int d, int e, int f, int g)
{
  int j = 0;
  volatile char x[50];
  &x;
          
  while(j < 2) {
    a++;
    j++;
  }

  return a;
}

int main(void)
{
  int i, j, k;
  i = j = k = 0;

  while (c++ < 10) {
    j = fun1(gk, k + 1, k + 2, k + 3, k + 4, k + 5, k + 6);
    printf ("%i\n", gk);
    if(j > 40)
      gk = j - 20;
    else
      gk = j;
  }
  report (gk);
  if (gk == 20)
    report(0xdeaddead);
  return (gk != 20);
}
