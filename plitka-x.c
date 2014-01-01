// gcc -I/usr/local/include/ -L/usr/local/lib -lX11 plitka-x.c ; ./a.out

#include <stdlib.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <unistd.h>

#define	NOD_MAX 128
#define	PLITKA_MAX 162			/* less than */
//#define	PLITKA_MAX 127			/* less than */

// 1280 800  2 4 5 8 10 16 20 32 40 80 160 1280

int main () {
  Display *dpy;
  Window root;
  GC gc1;

  dpy = XOpenDisplay (getenv ("DISPLAY"));
  root = DefaultRootWindow (dpy);
  gc1 = XCreateGC (dpy, root, 0, NULL);
  int waw = DisplayWidth(dpy, 0),
      wah = DisplayHeight(dpy, 0),
      xpos, ypos, rsz, i, nod_max,
      nod[NOD_MAX], nodsum[NOD_MAX];
  int nod_uniform[wah*waw], nod_uniform_max;
  int j, k;
  j = k = wah;

  for (i = NOD_MAX; i--; nod[i] = 0);

  // 1280 800   2 4 5 8 10 16 20 32 40 80 160
  // 1280 1024	2 4 8 16 32 64 128 256

  nod[0] = 0;
  for (i = rsz = 1; rsz++<waw; ) {
//    for (; ((waw%rsz) || (wah%rsz)) && (rsz<waw); rsz++) {}
    for (; ((waw%rsz)) && (rsz<waw); rsz++) {}
    nod[i++%NOD_MAX] = rsz;
  }

  nod_max = i - 1;
  printf("\nplitka sizes %d\n", nod_max);
  for (i = 0; i <= nod_max; i++)
    printf("{%d,%d} ", i, nod[i]);

  for (i = nod_max; nod[--i] > PLITKA_MAX;);
  nod_max = i;

  printf("\nplitka sizes filtered set %d\n", nod_max);
  for (i = 0; i <= nod_max; i++)
    printf("{%d,%d} ", i, nod[i]);

  printf("\n\nY-position\n");
  nodsum[0] = nod[0];
  nodsum[nod_max+1] = 0;

  for (i = 1; i <= nod_max; i++) {
    nodsum[i] = nodsum[i-1] + nod[i];
    printf("%2d %d\t+ %d =\t%d\n", i, nod[i], nodsum[i-1], nodsum[i]);
  }

  // uniformity

  nod_uniform_max = k = 0;
  for (i = nod_max+1; --i; ) {
    for (j = nod[nod_max] / nod[i]; j--;) {
      nod_uniform[k++] = i;
    }

  }
  nod_uniform_max = k;
  XFillRectangle(dpy, root, gc1, 1, 1, 1280, 800);

//#define	CVETA			/*  */
#ifdef CVETA
  int cveta3[3] = {
    0x20f000,
    0xffff00,
    0xc000c0,
  };
#endif
  while (1) {
    i = nod_uniform[rand()%(nod_uniform_max-3)];
    xpos = rand()%waw;
    xpos -= xpos%nod[i];
    ypos = nodsum[i];
    rsz = nod[i];

#ifdef CVETA
    j = rand()%3;
    XSetForeground(dpy, gc1, cveta3[j]);
#else
    XSetForeground(dpy, gc1, rand()%0xffFFff);
#endif
    XFillRectangle(dpy, root, gc1, xpos, (wah-nodsum[nod_max])+ypos-rsz-16, rsz-1, rsz-1);
    XFlush(dpy);
    usleep (999);
  }
//    XCloseDisplay (dpy);
//    return 0;
}
