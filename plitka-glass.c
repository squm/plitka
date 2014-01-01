#include	<stdlib.h>
#include	<windows.h>
#include	<stdio.h>
#include	<signal.h>
#include	<math.h>
LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);
//#define DEBUG 1		/* print debug info */
typedef unsigned int nod;
//let g:C_Libs='-mwindows'
struct nod_Info {
  nod
   *nods,		/* denotes array with sizes of nods */
   *nods_uniform,	/* array with occurence probability due to sizes */
   *plitka_offset,	/* screen y-coordinates for each node size */
   nod_cnt,		/* number of nods */
   nod_sum,		/* screen y-coordinate sum */
   nod_sum_uniform;     /* number of elements in nods_uniform array */
  HDC context, source;
  HWND hwnd;
  HBITMAP *plitka_bmps;
  DWORD *bmpdata;
  DWORD *sourcearray;
  DWORD **temporary, **scanline;
  HANDLE hHeap;
} Nods;
  void
xfree(LPVOID p) {
  if (p != NULL)
    HeapFree(Nods.hHeap, 0, p);
}
  void
repquit(const char *const msg) {
  unsigned int i;
  char* lpMsgBuf;
  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL);
  fprintf ( stderr, "Error: %s\n%s", msg, lpMsgBuf );
  DeleteDC(Nods.context);
  DeleteDC(Nods.source);
  ReleaseDC(Nods.hwnd, Nods.source);
  for (i = Nods.nod_cnt; i--; )
    DeleteObject(Nods.plitka_bmps[i]);
  xfree(Nods.scanline);
  xfree(Nods.sourcearray);
  xfree(Nods.nods);
  xfree(Nods.nods_uniform);
  xfree(Nods.bmpdata);
  xfree(Nods.plitka_bmps);
  xfree(Nods.plitka_offset);
  if (GetProcessHeap() != Nods.hHeap)
    HeapDestroy(Nods.hHeap);
  exit (EXIT_FAILURE);
}
  void
repsig(int sig) {
  switch (sig) {
  case SIGABRT:
    repquit("abnormal termination");
    break;
  case SIGINT:
    repquit("interactive attention signal");
    break;
  case SIGSEGV:
    repquit("invalid access to storage");
    break;
  case SIGTERM:
    repquit("termination request sent to the program");
    break;
  default:
    repquit("Exit");
  }
}
  LPVOID
xmalloc(size_t size) {
#if DEBUG
  printf("   ask %d\n", size);
#endif
  const size_t granularity[12] =
    { 1, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 1048576, 8388608};
  int i;
  for (i = 11; size <= granularity[i]; i--);
  if (size <= granularity[i+1])
    size = granularity[i+1];
#ifdef DEBUG
  printf("alloc (%d)\n", size);
#endif
  register LPVOID pointer =
   HeapAlloc(Nods.hHeap, HEAP_ZERO_MEMORY | HEAP_NO_SERIALIZE, size);
  if (pointer == NULL)
    repquit("Process memory exhausted.");
  return pointer;
}
  LPVOID
xrealloc(LPVOID original, size_t size) {
#if DEBUG
  printf("R  ask %d\n", size);
#endif
  const size_t granularity[12] =
    { 1, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 1048576, 8388608};
  int i;
  for (i = 11; size <= granularity[i]; i--);
  if (size <= granularity[i+1])
    size = granularity[i+1];
#ifdef DEBUG
  printf("  alloc R %9d  in %p\n", size, original);
#endif
  register LPVOID pointer =
   HeapReAlloc(Nods.hHeap, HEAP_ZERO_MEMORY, original, size);
  if (pointer == NULL)
    repquit("Memory exhausted");
  return pointer;
}
  void
mknods(const unsigned int waw, const unsigned int wah) {
  const unsigned int NPLITKA_SIZE_MAX = waw / 8;                /* less than */
  const unsigned int NPLITKA_SIZE_MIN = 2;                      /* skip small */
  unsigned int i, iu, umax;
  Nods.nod_sum = Nods.nod_cnt = 0;
  // TODO induct
  Nods.nods = (nod *) xmalloc(waw / NPLITKA_SIZE_MIN * sizeof(nod));
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
  Nods.plitka_bmps = (HBITMAP *) xmalloc(Nods.nod_cnt * sizeof(HBITMAP));
  Nods.plitka_offset = (nod *) xmalloc(Nods.nod_cnt * sizeof(nod) * 1);
  Nods.plitka_offset[0] = 0;
  for (i = 0; i < Nods.nod_cnt; i++) {
    Nods.plitka_bmps[i] =
     CreateCompatibleBitmap(Nods.source, Nods.nods[i], Nods.nods[i]);
    Nods.plitka_offset[i+1] = Nods.plitka_offset[i] + Nods.nods[i];
  }
}
  void
mkcolour(int x, int y, int wx, int wy, DWORD *const aye) {
  const char pattern = 1 + rand()%6;            /* exclude 000, 111 */
  int ix, iy;
  for (ix = wx; ix--; )
    for (iy = wy; iy--;)
      aye[iy * wx + ix] =
       RGB(
	   pattern&1 ? GetRValue(Nods.scanline[y+iy][x+ix]) : 255,
	   pattern&2 ? GetGValue(Nods.scanline[y+iy][x+ix]) : 255,
	   pattern&4 ? GetBValue(Nods.scanline[y+iy][x+ix]) : 255
	  );
}
  int
WINAPI WinMain (HINSTANCE wp1, HINSTANCE wp2, PSTR wp3, int wp4) {
  Nods.hHeap = GetProcessHeap();
  if (Nods.hHeap == NULL) {
    fprintf(stderr, "Process memory error\n");
    Nods.hHeap = HeapCreate(HEAP_NO_SERIALIZE, 8 * 1024 * 1024, 0);
    if (Nods.hHeap == NULL)
      repquit("Process memory error");
  }
  signal(SIGINT, repsig);
  signal(SIGTERM, repsig);      
  signal(SIGSEGV, repsig);      
  InvalidateRect(0, 0, WM_ERASEBKGND);

  Nods.hwnd = GetDesktopWindow();
  if (!LockWindowUpdate(Nods.hwnd))
    repquit("Window update not locked\n");
  const
   int waw = GetSystemMetrics(SM_CXSCREEN),
       wah = GetSystemMetrics(SM_CYSCREEN);

  Nods.source = GetDCEx(Nods.hwnd, NULL,
			DCX_CACHE | DCX_LOCKWINDOWUPDATE | DCX_CLIPCHILDREN);

  mknods(waw, wah);
  const int wah2 = Nods.nod_sum;                /* wah shrunk */
  Nods.context = CreateCompatibleDC(Nods.source);
  Nods.bmpdata = (DWORD *) xmalloc(Nods.nods[Nods.nod_cnt - 1] *
				   Nods.nods[Nods.nod_cnt - 1] *
				   sizeof(DWORD));
  Nods.temporary = Nods.scanline = NULL;
  Nods.sourcearray = NULL;

  Nods.temporary = (PDWORD*) xmalloc(wah2 * sizeof(PDWORD));

  Nods.sourcearray = (PDWORD) xmalloc(wah2 * waw * sizeof(DWORD));
  HBITMAP bmp = CreateCompatibleBitmap(Nods.source, waw, wah2);
  SelectObject(Nods.context, bmp);
  BitBlt(Nods.context, 0, 0, waw, wah2, Nods.source, 0, (wah - wah2), SRCCOPY);
  GetBitmapBits(bmp, waw * wah2 * sizeof(DWORD), Nods.sourcearray);
  DeleteObject(bmp);
  int i, xpos, ypos;
  for (i = wah2; i--;)                /* wah-1 .. 0 */
    Nods.temporary[i] = Nods.sourcearray + (i * waw);
  LockWindowUpdate(NULL);

  Nods.scanline = Nods.temporary;

  nod in,                                    /* plitka index */
      ips;                                   /* plitka size */

  MSG msg;
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
      mkcolour(xpos, ypos, ips, ips, Nods.bmpdata);
      SetBitmapBits(Nods.plitka_bmps[in], ips * ips * sizeof(DWORD),
		    (const PVOID) Nods.bmpdata);
      SelectObject(Nods.context, Nods.plitka_bmps[in]);
      BitBlt(Nods.source, xpos, ypos + (wah - wah2),
	     ips-1, ips-1, Nods.context, 0, 0, SRCCOPY);
    }
  }

  DeleteDC(Nods.context);
  DeleteDC(Nods.source);
  ReleaseDC(Nods.hwnd, Nods.source);
  for (i = Nods.nod_cnt; i--; )
    DeleteObject(Nods.plitka_bmps[i]);
  free(Nods.scanline);
  free(Nods.sourcearray);
  free(Nods.nods);
  free(Nods.nods_uniform);
  free(Nods.bmpdata);
  free(Nods.plitka_bmps);
  free(Nods.plitka_offset);
  if (GetProcessHeap() != Nods.hHeap)
    HeapDestroy(Nods.hHeap);
  return 0;
}
