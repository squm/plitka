#include	<stdlib.h>
#include	<windows.h>
#include	<stdio.h>
#include	<signal.h>
#include	<math.h>
//#define	DEBUG
//let g:C_Libs='-mwindows'
//typedef CONST void *PCVOID,*LPCVOID;
//LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);
typedef unsigned int nod;
struct nod_Info {
  nod
   *nods,		/* denotes array with sizes of nods */
   *nods_uniform,	/* array with occurence probability due to sizes */
   *plitka_offset,	/* screen y-coordinates for each node size */
   nod_cnt,		/* number of nods */
   nod_sum,		/* screen y-coordinate sum */
   nod_sum_uniform;     /* number of elements in nods_uniform array */;
  HDC context, source;
  HWND hwnd;
  HANDLE hHeap;		/* GetProcessHeap() */
} Nods;
  void
xfree(LPVOID p) {
  if (p != NULL)
    HeapFree(Nods.hHeap, 0, p);
}
  void
repquit(const char *const msg) {
  char* lpMsgBuf;
  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL);
  fprintf(stderr, "Error: %s\n%s", msg, lpMsgBuf );
  DeleteDC(Nods.context);
  DeleteDC(Nods.source);
  ReleaseDC(Nods.hwnd, Nods.source);
  xfree(Nods.nods);
  xfree(Nods.nods_uniform);
  xfree(Nods.plitka_offset);
  if (GetProcessHeap() != Nods.hHeap)
    HeapDestroy(Nods.hHeap);
  exit(EXIT_FAILURE);
}
  LPVOID
xmalloc(size_t size) {
//  if (size < 1024) size = 1024;
#ifdef DEBUG
  printf("alloc (%d)\n", size);
#endif
//  if (size > 0x7ff8) repquit("Memory demands too much."); // nongrowable
  LPVOID pointer =
   HeapAlloc(Nods.hHeap, HEAP_ZERO_MEMORY | HEAP_NO_SERIALIZE, size);
  if (pointer == NULL)
    repquit("Process memory exhausted.");
  return pointer;
}
  LPVOID
xrealloc(LPVOID original, size_t size) {
#ifdef DEBUG
  printf("  alloc R %-9d  in %p\n", size, original);
#endif
  LPVOID pointer = HeapReAlloc(Nods.hHeap, HEAP_ZERO_MEMORY, original, size);
  if (pointer == NULL)
    repquit("Memory exhausted");
#ifdef DEBUG
  printf("  alloc R %-9d out %p\n", size, pointer);
#endif
  return pointer;
}
  void
mknods(const unsigned int waw, const unsigned int wah) {
  const unsigned int NPLITKA_SIZE_MAX = waw / 8;                /* less than */
  const unsigned int NPLITKA_SIZE_MIN = 2;                      /* skip small */
  unsigned int i, iu, umax;
  Nods.nod_sum = Nods.nod_cnt = 0;
  // TODO induct this
  Nods.nods = (nod *) xmalloc(waw * sizeof(nod));
  for (i = NPLITKA_SIZE_MIN; i++ < waw;) {
    if (waw % i)
      continue;
    if (Nods.nod_sum > wah / 3)
      break;
    if (i > NPLITKA_SIZE_MAX)
      break;
    Nods.nod_sum += i;
    Nods.nods[Nods.nod_cnt++] = i;
  }
  Nods.nods =
   (nod *) xrealloc
   ((LPVOID) Nods.nods, Nods.nod_cnt * sizeof(nod)); 
//  Nods.nods_uniform = (nod *) xmalloc(Nods.nod_sum * sizeof(nod));
  Nods.nods_uniform = (nod *) xmalloc(8 * Nods.nod_sum * sizeof(nod));
  nod ndiv;
  umax = 0;
  Nods.nod_sum_uniform = 0;
  for (i = Nods.nod_cnt; i--;) {
//    ndiv = Nods.nods[Nods.nod_cnt-1] / Nods.nods[i];
    ndiv = 10.0 * Nods.nods[Nods.nod_cnt-1] / Nods.nods[i];
    Nods.nod_sum_uniform += ndiv;
    for (iu = ndiv; iu--; Nods.nods_uniform[umax++] = i);
  }
#if 0
  umax = 0;
  for (i = Nods.nod_cnt; i--;) {
//    ndiv = Nods.nods[Nods.nod_cnt-1] / Nods.nods[i];
    ndiv = 10 * (double) Nods.nods[Nods.nod_cnt-1] / Nods.nods[i];
    fprintf(stderr, "\n%-2u* %3u / %-3u = %3u",
	    i, Nods.nods[i], ndiv, Nods.nods_uniform[umax]);
    for (iu = ndiv; iu--; ) {
      fprintf(stderr, "%2u,", Nods.nods_uniform[umax++]);
    }
  }
  fprintf(stderr, "\nTotal: %u (%u-%u sum)\n",
	  Nods.nod_cnt, Nods.nod_sum, Nods.nod_sum_uniform);
#endif
  Nods.nods_uniform =
   (nod *) xrealloc
   ((LPVOID)Nods.nods_uniform, Nods.nod_sum_uniform * sizeof(nod));
  Nods.plitka_offset = (nod *) xmalloc(Nods.nod_cnt * sizeof(nod));
  Nods.plitka_offset[0] = 0;
  for (i = 0; i < Nods.nod_cnt; i++) {
    Nods.plitka_offset[i+1] = Nods.plitka_offset[i] + Nods.nods[i];
  }
}
  void
repsig(int sig) {
  switch (sig) {
  case SIGABRT: {
		  repquit("abnormal termination");
		  break;
		}
  case SIGINT: {
		 repquit("interactive attention signal");
		 break;
	       }
  case SIGSEGV: {
		  repquit("invalid access to storage");
		  break;
		}
  case SIGTERM: {
		  repquit("termination request sent to the program");
		  break;
		}
  default: {
		repquit("Exit");
	   }
  }
}
  int
WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPInst, PSTR szCmd, int iShow) {
  Nods.hHeap = GetProcessHeap();
  if (Nods.hHeap == NULL)
    repquit("Process memory error");
  signal(SIGINT, repsig);
  signal(SIGTERM, repsig);
  signal(SIGSEGV, repsig);
  InvalidateRect(0, 0, WM_ERASEBKGND);

  Nods.hwnd = GetDesktopWindow();
  const
   int waw = GetSystemMetrics(SM_CXSCREEN),
       wah = GetSystemMetrics(SM_CYSCREEN);

  Nods.source = GetDCEx(Nods.hwnd, NULL,
		   DCX_CACHE | DCX_LOCKWINDOWUPDATE | DCX_CLIPCHILDREN);

  mknods(waw, wah);
  const int wah2 = Nods.nod_sum;                /* wah shrunk */
  Nods.context = CreateCompatibleDC(Nods.source);

  nod in,                                    /* plitka index */
      ips;                                   /* plitka size */
  int xpos, ypos;

  MSG msg;
  RECT Rect;
  unsigned char r, g, b, stdev;
  unsigned char const threshold = 15;
  HBRUSH hBrush;
  while (1) {
    if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT)
	break;
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    else {
      Sleep(5);
      in = Nods.nods_uniform[rand() % Nods.nod_sum_uniform];
      ips = Nods.nods[in];
      xpos = rand()%waw;
      xpos = xpos - xpos % ips;
      ypos = Nods.plitka_offset[in];
      SetRect(&Rect,
	      xpos,		ypos + (wah - wah2),
	      xpos + ips - 1,	ypos + ips + (wah - wah2) - 1);
      do {
	r = 0x55 + rand()%0xaa;
	g = 0x55 + rand()%0xaa;
	b = 0x55 + rand()%0xaa;
	stdev = (r + g + b) / 3;
      }
      while (
	     abs(r - stdev) <= threshold ||
	     abs(g - stdev) <= threshold ||
	     abs(b - stdev) <= threshold
	    );
      hBrush = CreateSolidBrush(RGB(r, g, b));
      FillRect(Nods.source, &Rect, hBrush);
      DeleteObject(hBrush);
    }
  }

  DeleteDC(Nods.context);
  DeleteDC(Nods.source);
  ReleaseDC(Nods.hwnd, Nods.source);
  xfree(Nods.nods);
  xfree(Nods.nods_uniform);
  xfree(Nods.plitka_offset);
  return 0;
}
