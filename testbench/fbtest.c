/* Simple frame buffer test. Draws some horizontal with different colors. */
#include "support.h"

#define BUFADDR   0x00100000
#define BASEADDR  0x97000000
#define BUF_PTR  ((unsigned char *)BUFADDR)
#define PAL_PTR  ((unsigned long *)(BASEADDR + 0x400))
#define SIZEX     640
#define SIZEY     480

#define putxy(x,y) *(BUF_PTR + (x) + (y) * SIZEX)
#define setpal(i,r,g,b) *(PAL_PTR + (i)) = (((unsigned long)(r) & 0xff) << 16) | (((unsigned long)(g) & 0xff) << 8) | (((unsigned long)(b) & 0xff) << 0)

void hline (int y, int x1, int x2, unsigned char c)
{
  int x;
  for (x = x1; x < x2; x++)
    putxy(x, y) = c;
}

int main(void)
{
  unsigned i;
  
  /* Set address of buffer at predefined location */
  *((unsigned long *)(BASEADDR) + 0x1) = BUFADDR;
  for (i = 0; i < 256; i++)
    setpal (i, 256 - i, i, 128 ^ i);

  /* Turn display on */  
  *((unsigned long *)(BASEADDR) + 0x0) = 0xffffffff;
    
  for (i = 0; i < 16; i++) {
    hline (i, 0, i, i);
    hline (256 - i, 256 - i, 256 + i, i);
  }
  
  report (0xdeaddead);
  return 0;
}
