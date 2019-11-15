#include "winpriv.h"
#include <stdio.h>
#include <string.h>
int TABBAR_HEIGHT=0;
static HWND empty_wnd;
static HWND empty_close_wnd;
static bool initialized = false;
static void
place_field(int * curpoi, int width, int * pospoi)
{
  if (* pospoi < 0) {
    * pospoi = * curpoi;
    (* curpoi) += width;
  }
}

#define EMPTYBARCLASS "EmptyBar"
static int wcswid(const wchar_t *s){
  return cell_width * wcslen(s);
}
static LRESULT CALLBACK
nothing_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
  printf("%p %p %p %p\n", &wp, &lp, &msg, &hwnd);
  //return 0;
  win_schedule_update();
  return DefWindowProcA(hwnd, msg, wp, lp);;
}
extern struct tabinfo {
  unsigned long tag;
  HWND wnd;
  wchar * title;
} * tabinfo;
extern int ntabinfo;
extern void refresh_tab_titles();
void
win_toggle_empty(bool show, bool focus)
{
  puts("toggle");
  RECT cr;
  GetClientRect(wnd, &cr);
  int width = cr.right - cr.left;

  int margin = cell_width / 6 + 1;
  int height = cell_height + margin * 2;
  TABBAR_HEIGHT= height;
  int pos_close = -1;
  int barpos = margin;
  int button_width = cell_width * 2;
  int ctrl_height = height - margin * 2;
  
  place_field(&barpos, button_width, &pos_close);
  //int edit_width = width - button_width * 3 - margin * 2;
  //int ctrl_height = height - margin * 2;
  //int sf_height = ctrl_height - 4;
  if (!initialized) {
      RegisterClassA(&(WNDCLASSA){
	  .style = 0,
				    .lpfnWndProc = nothing_proc,
				    .cbClsExtra = 0,
				    .cbWndExtra = 0,
				    .hInstance = inst,
				    .hIcon = NULL,
				    .hCursor = NULL,
				    .hbrBackground = (HBRUSH)(COLOR_3DFACE + 1),
				    .lpszMenuName = NULL,
				    .lpszClassName = EMPTYBARCLASS
      });
      initialized = true;
  }
  empty_wnd = CreateWindowExA(0, EMPTYBARCLASS, "", WS_CHILD, 0, 0, 0, 0, wnd, 0, inst, NULL);
  refresh_tab_titles();

  int pos_start = margin;
  for (int i = 0; i < ntabinfo; i ++) {
    int width = wcswid(tabinfo[i].title);
    empty_close_wnd = CreateWindowExW(0, W("BUTTON"), tabinfo[i].title, WS_CHILD | WS_VISIBLE,
				      pos_start, margin, width, ctrl_height,
				      empty_wnd, NULL, inst, NULL);
    pos_start += width + margin;
  }
  SetWindowPos(empty_wnd, 0,
	       cr.right - width, 0,//cr.bottom - height,
	       width, height,
	       SWP_NOZORDER);
  show = show || focus;
  ShowWindow(empty_wnd, SW_SHOW);
  win_adapt_term_size(false, false);
}
bool win_empty_visible(){
  return initialized;
}
