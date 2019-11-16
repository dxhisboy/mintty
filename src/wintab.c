#include "winpriv.h"
#include <stdio.h>
#include <string.h>
int TABBAR_HEIGHT=0;
static HWND empty_wnd;
//static HWND empty_close_wnd;
static HFONT empty_font;
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
/* static int wcswid(const wchar_t *s){ */
/*   return (cell_width * 1.6) * wcslen(s); */
/* } */
extern void refresh_tab_titles(bool);
extern struct tabinfo {
  unsigned long tag;
  HWND wnd;
  wchar * title;
} * tabinfo;
extern int ntabinfo;

static LRESULT CALLBACK
nothing_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
  printf("%p %p %p %p\n", &wp, &lp, &msg, &hwnd);
  if (msg == WM_PAINT){
    int tab_width = 100;
    int height = cell_height + (cell_width / 6 + 1) * 4;
    PAINTSTRUCT pntS;
    HDC dc = BeginPaint(empty_wnd, &pntS);
    HPEN pen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
    SetTextColor(dc, RGB(0, 0, 0));
    SetTextAlign(dc, TA_CENTER);
    SelectObject(dc, pen);
    refresh_tab_titles(true);
    int margin = cell_width / 6 + 1;
    for (int i = 0; i < ntabinfo; i ++){
      MoveToEx(dc, i * tab_width + margin, height, NULL);
      LineTo(dc, i * tab_width + margin, 0);
      LineTo(dc, (i + 1) * tab_width + margin, 0);
      TextOutW(dc, i * tab_width + tab_width/2 + margin, (height - cell_height) / 2, tabinfo[i].title, min(wcslen(tabinfo[i].title), 15));
    }
    MoveToEx(dc, ntabinfo * tab_width + margin, height, NULL);
    LineTo(dc, ntabinfo * tab_width + margin, 0);
    EndPaint(wnd, &pntS);
  }

  //return 0;
  //win_schedule_update();
  return DefWindowProcA(hwnd, msg, wp, lp);;
}

void
win_toggle_empty(bool show, bool focus)
{
  puts("toggle");
  RECT cr;
  GetClientRect(wnd, &cr);
  int width = cr.right - cr.left;

  int margin = cell_width / 6 + 1;
  int height = cell_height + margin * 4;
  TABBAR_HEIGHT= height;
  int pos_close = -1;
  int barpos = margin;
  int button_width = cell_width * 2;
  //int ctrl_height = cell_height + margin * 2;
  
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

  empty_font = CreateFontW(cell_height, cell_width, 0, 0, FW_DONTCARE, false, false, false,
			   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			   DEFAULT_QUALITY, FIXED_PITCH | FF_DONTCARE,
			   cfg.font.name);
  SendMessage(empty_wnd, WM_SETFONT, (WPARAM)empty_font, 1);
  //HDC dc = CreateCompatibleDC(dc);
  /* SetBkMode(dc, RGB(0, 0, 0)); */
  int pos_start = margin;
  for (int i = 0; i < ntabinfo; i ++) {
    
    /* int text_extent = GetTabbedTextExtentW(dc, tabinfo[i].title, wcslen(tabinfo[i].title), 0, 0); */
    /* int width = (text_extent & 0xFFFF) + margin * 2; */
    /* int height = (text_extent >> 16) + margin * 2; */
    /* empty_close_wnd = CreateWindowExW(0, W("BUTTON"), tabinfo[i].title, WS_CHILD | WS_VISIBLE, */
    /* 				      pos_start, margin, width, height, */
    /* 				      empty_wnd, NULL, inst, NULL); */
    /* SendMessage(empty_close_wnd, WM_SETFONT, (WPARAM)empty_font, 1); */
    /* HDC dcnew = GetDC(empty_close_wnd); */
    /* int text_extent_new = GetTabbedTextExtentW(dcnew, tabinfo[i].title, wcslen(tabinfo[i].title), 0, 0); */
    /* printf("%x %x\n", text_extent, text_extent_new); */
    pos_start += width + margin;
  }
  show = show || focus;
  ShowWindow(empty_wnd, SW_SHOW);
  win_adapt_term_size(false, false);

  SetWindowPos(empty_wnd, 0,
	       cr.right - width, 0,//cr.bottom - height,
	       width, height,
	       SWP_NOZORDER);
}
bool win_empty_visible(){
  return initialized;
}
