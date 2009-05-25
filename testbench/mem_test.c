/* Simple test, which tests whether memory accesses are performed correctly.
   WARNING: Requires big endian host!!! */

#include "support.h"

unsigned long _ul, *pul;
unsigned short *pus;
unsigned char *puc;

int main ()
{
  unsigned long cnt = 0;
  _ul = 0x12345678;
  pul = &_ul;
  report (*pul);
  cnt = (cnt + *pul) << 1;

  pus = (unsigned short *)&_ul;
  report (*pus);
  cnt = (cnt + *pus) << 1;
  pus++;
  report (*pus);
  cnt = (cnt + *pus) << 1;
  
  puc = (unsigned char *)&_ul;
  report (*puc);
  cnt = (cnt + *puc) << 1;
  puc++;
  report (*puc);
  cnt = (cnt + *puc) << 1;
  puc++;
  report (*puc);
  cnt = (cnt + *puc) << 1;
  puc++;
  report (*puc);
  cnt = (cnt + *puc) << 1;
  
  *pul = 0xdeaddead;
  report (*pul);
  cnt = (cnt + *pul) << 1;
  
  pus = (unsigned short *)&_ul;
  *pus = 0x5678;
  report (*pul);
  cnt = (cnt + *pul) << 1;
  pus++;
  *pus = 0x1234;
  report (*pul);
  cnt = (cnt + *pul) << 1;
  
  puc = (unsigned char *)&_ul;
  *puc = 0xdd;
  report (*pul);
  cnt = (cnt + *pul) << 1;
  puc++;
  *puc = 0xcc;
  report (*pul);
  cnt = (cnt + *pul) << 1;
  puc++;
  *puc = 0xbb;
  report (*pul);
  cnt = (cnt + *pul) << 1;
  puc++;
  *puc = 0xaa;
  report (*pul);
  cnt = (cnt + *pul) << 1;
  
  report (cnt);
  cnt ^= 0xdeaddead ^ 0xda25e544;
  report(cnt);
  return (cnt != 0xdeaddead);
}
