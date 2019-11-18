#include "winpriv.h"
#include <stdio.h>
#include <string.h>
int TABBAR_HEIGHT = 0;
int prev_height = 0;
HWND tabbar_wnd;
HPEN tabbar_pen, tabbar_cur_pen;
HBRUSH tabbar_brush, tabbar_cur_brush;
//static HWND tabbar_close_wnd;
/* #define tabbar_fg_colour GetSysColor(COLOR_INACTIVECAPTIONTEXT) //cfg.ansi_colours[0] */
/* #define tabbar_bg_colour GetSysColor(COLOR_INACTIVECAPTION) //cfg.ansi_colours[10] */
/* #define tabbar_br_colour GetSysColor(COLOR_INACTIVEBORDER) //cfg.ansi_colours[10] */
/* #define tabbar_cur_bg_colour GetSysColor(COLOR_ACTIVECAPTION) */
/* #define tabbar_cur_fg_colour GetSysColor(COLOR_CAPTIONTEXT) */
/* #define tabbar_cur_br_colour GetSysColor(COLOR_ACTIVEBORDER) //cfg.ansi_colours[10] */

#define tabbar_fg_colour GetSysColor(COLOR_BTNTEXT) //cfg.ansi_colours[0]
#define tabbar_bg_colour GetSysColor(COLOR_BTNFACE) //cfg.ansi_colours[10]
#define tabbar_br_colour 0xd9d9d9 //GetSysColor(COLOR_WINDOWFRAME) //cfg.ansi_colours[10]
#define tabbar_cur_fg_colour GetSysColor(COLOR_BTNTEXT)
#define tabbar_cur_bg_colour GetSysColor(COLOR_BTNHIGHLIGHT)
#define tabbar_cur_br_colour 0xd9d9d9 //cfg.ansi_colours[10]

//static int tabbar_charw[WCHAR_MAX - WCHAR_MIN];
static HFONT tabbar_font;
static bool initialized = false;
int max_tab_width = 300;
int min_tab_width = 20;
#define TABBARCLASS "TabBar"
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

static int fit_title(HDC dc, RECT textrect, wchar_t *title_in, wchar_t *title_out, int olen){
  int title_len = wcslen(title_in);
  SIZE text_size;
  GetTextExtentPoint32W(dc, title_in, title_len, &text_size);
  int text_width = text_size.cx;
  if (text_width <= textrect.right - textrect.left) {
    wcsncpy(title_out, title_in, olen);
    return title_len;
  }
  printf("%d %d\n", text_width, textrect.right - textrect.left);
  wcsncpy(title_out, L"**", olen);
  title_out[0] = title_in[0];
  GetTextExtentPoint32W(dc, title_out, 2, &text_size);
  text_width = text_size.cx;
  int i;
  for (i = title_len - 1; i > 1; i --) {
    int charw;
    GetCharWidth32W(dc, title_in[i], title_in[i], &charw);
    if (text_width + charw <= textrect.right - textrect.left)
      text_width += charw;
    else
      break;
  }
  wcsncpy(title_out + 2, title_in + i + 1, olen - 2);
  return wcsnlen(title_out, olen);
}
static void paint_tab(HDC dc, int tab_width, int tab_height, int i){
  int margin = cell_width / 6 + 1;
  int padding = margin * 2;
  if (tabinfo[i].wnd == wnd) {
    SetTextColor(dc, tabbar_cur_fg_colour);
    SelectObject(dc, tabbar_cur_brush);
    SelectObject(dc, tabbar_cur_pen);
  } else {
    SetTextColor(dc, tabbar_fg_colour);
    SelectObject(dc, tabbar_brush);
    SelectObject(dc, tabbar_pen);
  }
  int left = tab_width * i;
  RECT textrect = {.left=left + margin, .right=left + tab_width - margin, .top = padding, .bottom = tab_height + padding};
  if (tabinfo[i].wnd == wnd) {
    Rectangle(dc, left - margin, padding - margin, left + tab_width + margin, tab_height + padding);
  } else {
    Rectangle(dc, left, padding, left + tab_width, tab_height + padding);
  }
  wchar_t title_fit[256];
  fit_title(dc, textrect, tabinfo[i].title, title_fit, 256);
  ExtTextOutW(dc, left + tab_width / 2, (tab_height - cell_height) / 2 + padding, ETO_CLIPPED, &textrect, title_fit, wcslen(title_fit), NULL);
}
static LRESULT CALLBACK
nothing_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
  RECT cr;
  GetClientRect(wnd, &cr);
  int win_width = cr.right - cr.left;

  printf("%p %p %p %p\n", &wp, &lp, &msg, &hwnd);
  if (msg == WM_PAINT){
    //int tab_width = 100;
    int height = cell_height + (cell_width / 6 + 1) * 2;
    PAINTSTRUCT pntS;
    HDC dc = BeginPaint(tabbar_wnd, &pntS);
    RECT cr;
    GetClientRect(tabbar_wnd, &cr);
    SetTextColor(dc, tabbar_fg_colour);
    SelectObject(dc, tabbar_font);
    SetBkMode(dc, TRANSPARENT);
    SetBkColor(dc, tabbar_fg_colour);
    SetTextAlign(dc, TA_CENTER | VTA_CENTER);
    SelectObject(dc, tabbar_pen);
    SelectObject(dc, tabbar_brush);

    int tab_width = win_width / ntabinfo;
    tab_width = min(tab_width, max_tab_width);
    tab_width = max(tab_width, min_tab_width);
    for (int i = 0; i < ntabinfo; i ++){
      paint_tab(dc, tab_width, height, i);
    }
    EndPaint(wnd, &pntS);
    return 0;
  }
  return DefWindowProcA(hwnd, msg, wp, lp);;
}

void
tabbar_init()
{
  //int margin = cell_width / 6 + 1;
  //TABBAR_HEIGHT = cell_height + margin * 2;
  tabbar_pen = CreatePen(PS_SOLID, 1, tabbar_br_colour);
  tabbar_cur_pen = CreatePen(PS_SOLID, 1, tabbar_cur_br_colour);
  tabbar_brush = CreateSolidBrush(tabbar_bg_colour);
  tabbar_cur_brush = CreateSolidBrush(tabbar_cur_bg_colour);
  RegisterClassA(&(WNDCLASSA){
                                .style = 0,
				.lpfnWndProc = nothing_proc,
				.cbClsExtra = 0,
 				.cbWndExtra = 0,
				.hInstance = inst,
				.hIcon = NULL,
				.hCursor = NULL,
				.hbrBackground = tabbar_brush,//(HBRUSH)(COLOR_3DFACE + 1),
				.lpszMenuName = NULL,
				.lpszClassName = TABBARCLASS
				});
  
  tabbar_wnd = CreateWindowExA(0, TABBARCLASS, "", WS_CHILD, 0, 0, 0, 0, wnd, 0, inst, NULL);

  tabbar_font = CreateFontW(cell_height, cell_width, 0, 0, FW_DONTCARE, false, false, false,
			    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			    DEFAULT_QUALITY, FIXED_PITCH | FF_DONTCARE,
			    cfg.font.name);
  //SendMessage(tabbar_wnd, WM_SETFONT, (WPARAM)tabbar_font, 1);
  /* HDC dc = GetDC(tabbar_wnd); */
  /* SelectObject(dc, tabbar_font); */
  /* printf("%d %d\n", WCHAR_MIN, WCHAR_MAX); */
  /* GetCharWidth32W(dc, WCHAR_MIN, WCHAR_MAX, tabbar_charw); */
  /* printf("%d %d\n", WCHAR_MIN, WCHAR_MAX); */
  /* ReleaseDC(tabbar_wnd, dc); */
  initialized = true;
}
void
tabbar_destroy(){
  DeleteObject(tabbar_pen);
  DeleteObject(tabbar_brush);
  DeleteObject(tabbar_cur_brush);
  DeleteObject(tabbar_font);
  DestroyWindow(tabbar_wnd);
  UnregisterClassA(TABBARCLASS, inst);
  initialized = false;
}
void
win_toggle_tabbar(bool show, bool focus)
{
  puts("toggle");
  RECT cr;
  GetClientRect(wnd, &cr);
  int width = cr.right - cr.left;

  int margin = cell_width / 6 + 1;
  int padding = margin * 2;
  int height = cell_height  + margin * 2;
  if (height != TABBAR_HEIGHT && initialized) {
    tabbar_destroy();
  }
  if (!initialized) {
    tabbar_init();
  }
  prev_height = height;
  //int pos_close = -1;
  //int barpos = margin;
  //int button_width = cell_width * 2;

  if (!initialized) {
    tabbar_init();
  }
  if (TABBAR_HEIGHT == 0){
    show = show || focus;
    TABBAR_HEIGHT = height + padding;
    SetWindowPos(tabbar_wnd, 0,
		 cr.right - width, 0,//cr.bottom - height,
		 width, height + padding,
		 SWP_NOZORDER);
    ShowWindow(tabbar_wnd, SW_SHOW);
  } else {
    TABBAR_HEIGHT = 0;
    ShowWindow(tabbar_wnd, SW_HIDE);
  }
  puts("colors");
  for (int i = 0; i < 31; i ++){
    printf("%d %x\n", i, GetSysColor(i));
  }
  win_adapt_term_size(false, false);
}
bool win_tabbar_visible(){
  return initialized;
}
