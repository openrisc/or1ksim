/* Test local and global variables */
#include "support.h"

int global = 5;

int func (unsigned long a, char b) {
  global = 2;
  a = 3 + a;
  b = 4 + b;
  return a + b;
}

int main () {
  int local = 7;
  global += func (local, 14);
  report (global ^ 0xdeaddead ^ 30);
  return 0;
}
