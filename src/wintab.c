#include "winpriv.h"
#include "Commctrl.h"
//#include <uxtheme.h>
//#include <vssym32.h>
#include <windowsx.h>
#include <stdio.h>
#include <string.h>
int TABBAR_HEIGHT = 0;
static int prev_height = 0;
static HWND tab_wnd, bar_wnd;

static HFONT tabbar_font;
static bool initialized = false;
static const int max_tab_width = 300;
static const int min_tab_width = 20;
static int prev_tab_width = 0;

#define TABBARCLASS "Tabbar"

extern struct tabinfo {
  unsigned long tag;
  HWND wnd;
  wchar * title;
} * tabinfo;
extern int ntabinfo;

static int
fit_title(HDC dc, int tab_width, wchar_t *title_in, wchar_t *title_out, int olen)
{
  int title_len = wcslen(title_in);
  SIZE text_size;
  GetTextExtentPoint32W(dc, title_in, title_len, &text_size);
  int text_width = text_size.cx;
  if (text_width <= tab_width) {
    wcsncpy(title_out, title_in, olen);
    return title_len;
  }
  wcsncpy(title_out, L"\u2026\u2026", olen);
  title_out[0] = title_in[0];
  GetTextExtentPoint32W(dc, title_out, 2, &text_size);
  text_width = text_size.cx;
  int i;
  for (i = title_len - 1; i > 1; i --) {
    int charw;
    GetCharWidth32W(dc, title_in[i], title_in[i], &charw);
    if (text_width + charw <= tab_width)
      text_width += charw;
    else
      break;
  }
  wcsncpy(title_out + 2, title_in + i + 1, olen - 2);
  return wcsnlen(title_out, olen);
}

static void
tabbar_update()
{
  RECT tab_cr;
  GetClientRect(tab_wnd, &tab_cr);
  int win_width = tab_cr.right - tab_cr.left;
  if (ntabinfo == 0) return;
  /* { */
  /*   SendMessage(wnd, WM_USER, 0, 4); */
  /* } */
  int tab_height = cell_height + (cell_width / 6 + 1) * 2;
  int tab_width = (win_width - 2 * tab_height) / ntabinfo;
  tab_width = min(tab_width, max_tab_width);
  tab_width = max(tab_width, min_tab_width);
  printf("width: %d %d %d\n", win_width, tab_width, ntabinfo);
  SendMessage(tab_wnd, TCM_SETITEMSIZE, 0, tab_width | tab_height << 16);
  TCITEMW tie;
  tie.mask = TCIF_TEXT | TCIF_IMAGE | TCIF_PARAM;
  tie.iImage = -1;
  wchar_t title_fit[256];
  HDC tabdc = GetDC(tab_wnd);
  SelectObject(tabdc, tabbar_font);
  tie.pszText = title_fit;
  SendMessage(tab_wnd, TCM_DELETEALLITEMS, 0, 0);
  //bool fg_ismintty = false;
  for (int i = 0; i < ntabinfo; i ++) {
    fit_title(tabdc, tab_width, tabinfo[i].title, title_fit, 256);
    tie.lParam = (LPARAM)tabinfo[i].wnd;
    SendMessage(tab_wnd, TCM_INSERTITEMW, i, (LPARAM)&tie);
    if (tabinfo[i].wnd == wnd) {
      SendMessage(tab_wnd, TCM_SETCURSEL, i, 0);
    }
  }
  ReleaseDC(tab_wnd, tabdc);
}

// To prevent heavy flickers.
static LRESULT CALLBACK
tab_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR uid, DWORD_PTR data)
{
  if (msg == WM_PAINT) {
    RECT rect;
    GetClientRect(hwnd, &rect);
    PAINTSTRUCT pnts;
    HDC hdc = BeginPaint(hwnd, &pnts);
    HDC bufdc = CreateCompatibleDC(hdc);
    HBITMAP bufbitmap = CreateCompatibleBitmap(hdc, rect.right - rect.left, rect.bottom - rect.top);
    SelectObject(bufdc, bufbitmap);
    SendMessage(hwnd, WM_ERASEBKGND, (WPARAM)bufdc, true);
    SendMessage(hwnd, WM_PRINT, (WPARAM)bufdc, PRF_CLIENT | PRF_NONCLIENT);
    BitBlt(hdc, 0, 0, rect.right - rect.left, rect.bottom - rect.top, bufdc, 0, 0, SRCCOPY);
    DeleteObject(bufbitmap);
    DeleteDC(bufdc);
    EndPaint(hwnd, &pnts);
    return true || uid || data;
  } else if (msg == WM_ERASEBKGND) {
    if (!lp) return true;
  } else if (msg == WM_DRAWITEM) {
    printf("Received drawitem");
  }
  return DefSubclassProc(hwnd, msg, wp, lp);
}

// We need to make a container for the tabbar for handling WM_NOTIFY, also for further extensions
static LRESULT CALLBACK
container_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
  printf("%p %p %p %p\n", &hwnd, &msg, &wp, &lp);

  if (msg == WM_NOTIFY) {
    LPNMHDR lpnmhdr = (LPNMHDR)lp;
    printf("notify %lld %d %d\n", lpnmhdr->idFrom, lpnmhdr->code, TCN_SELCHANGE);
    if (lpnmhdr->code == TCN_SELCHANGE) {
      int isel = SendMessage(tab_wnd, TCM_GETCURSEL, 0, 0);
      TCITEMW tie;
      tie.mask = TCIF_PARAM;
      SendMessage(tab_wnd, TCM_GETITEM, isel, (LPARAM)&tie);
      printf("%p\n", (void*)tie.lParam);
      RECT rect_me;
      GetWindowRect(wnd, &rect_me);
      printf("%d %d %d %d\n", rect_me.left, rect_me.right, rect_me.top, rect_me.bottom);
      //ShowWindow((HWND)tie.lParam, SW_RESTORE);
      //ShowWindow((HWND)tie.lParam, SW_SHOW);
      //SetForegroundWindow((HWND)tie.lParam);
  // SetActiveWindow(top_wnd);

      //if (IsIconic((HWND)tie.lParam))
      //ShowWindow((HWND)tie.lParam, SW_RESTORE);
      win_to_top((HWND)tie.lParam);
      //SetForegroundWindow((HWND)tie.lParam);
      /* SetWindowPos((HWND)tie.lParam, 0, rect_me.left, rect_me.top, rect_me.right - rect_me.left, rect_me.bottom - rect_me.top, SWP_SHOWWINDOW); */
      //PostMessage((HWND)tie.lParam, WM_SIZE, 0, 0);
      for (int i = 0; i < ntabinfo; i ++) {
        if (tabinfo[i].wnd == wnd)
          SendMessage(tab_wnd, TCM_SETCURSEL, i, 0);
      }
    }
  }
  else if (msg == WM_CREATE) {
    puts("create");
    tab_wnd = CreateWindowExA(0, WC_TABCONTROL, "", WS_CHILD|TCS_FIXEDWIDTH|TCS_OWNERDRAWFIXED, 0, 0, 0, 0, hwnd, 0, inst, NULL);
    SetWindowSubclass(tab_wnd, tab_proc, 0, 0);
    tabbar_font = CreateFontW(cell_height * 4 / 5, cell_width * 4 / 5, 0, 0, FW_DONTCARE, false, false, false,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              DEFAULT_QUALITY, FIXED_PITCH | FF_DONTCARE,
                              cfg.font.name);
    SendMessage(tab_wnd, WM_SETFONT, (WPARAM)tabbar_font, 1);
  }
  else if (msg == WM_SHOWWINDOW) {
    if (wp) {
      printf("show %p\n", bar_wnd);
      ShowWindow(tab_wnd, SW_SHOW);
      tabbar_update();
    }
    //return true;
  }
  else if (msg == WM_SIZE) {
    tabbar_font = CreateFontW(cell_height * 4 / 5, cell_width * 4 / 5, 0, 0, FW_DONTCARE, false, false, false,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              DEFAULT_QUALITY, FIXED_PITCH | FF_DONTCARE,
                              cfg.font.name);
    SendMessage(tab_wnd, WM_SETFONT, (WPARAM)tabbar_font, 1);

    SetWindowPos(tab_wnd, 0,
                 0, 0,
                 lp & 0xffff, lp >> 16,
                 SWP_NOZORDER);
    tabbar_update();
  } else if (msg == WM_DRAWITEM) {
    LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lp;
    HDC hdc = dis->hDC;
    printf("Container Received drawitem %llx %p\n", wp, dis);
    int hcenter = (dis->rcItem.left + dis->rcItem.right) / 2;
    int vcenter = (dis->rcItem.top + dis->rcItem.bottom) / 2;

    SetTextAlign(hdc, TA_CENTER|TA_TOP);
    TCITEMW tie;
    wchar_t buf[256];
    tie.mask = TCIF_TEXT;
    tie.pszText = buf;
    tie.cchTextMax = 256;
    SendMessage(tab_wnd, TCM_GETITEMW, dis->itemID, (LPARAM)&tie);
    if (tabinfo[dis->itemID].wnd == wnd){
      FillRect(hdc, &dis->rcItem, GetSysColorBrush(COLOR_GRADIENTACTIVECAPTION));
    } else {
      FillRect(hdc, &dis->rcItem, GetSysColorBrush(COLOR_3DFACE));
    }
    SetBkMode(hdc, TRANSPARENT);
    TextOutW(hdc, hcenter, vcenter - cell_height / 3, tie.pszText, wcslen(tie.pszText));
  }

  return CallWindowProc(DefWindowProc, hwnd, msg, wp, lp);
}

static void
tabbar_init()
{
  RegisterClassA(&(WNDCLASSA) {
      .style = 0,
                                .lpfnWndProc = container_proc,
                                .cbClsExtra = 0,
                                .cbWndExtra = 0,
                                .hInstance = inst,
                                .hIcon = NULL,
                                .hCursor = NULL,
                                .hbrBackground = NULL, //(HBRUSH)(COLOR_3DFACE + 1),
                                .lpszMenuName = NULL,
                                .lpszClassName = TABBARCLASS
                                });
  bar_wnd = CreateWindowExA(WS_EX_STATICEDGE, TABBARCLASS, "", WS_CHILD|WS_BORDER, 0, 0, 0, 0, wnd, 0, inst, NULL);

  initialized = true;
}

static void
tabbar_destroy()
{
  DestroyWindow(tab_wnd);
  DestroyWindow(bar_wnd);
  initialized = false;
}

static void
win_toggle_tabbar(bool show)
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
  tabbar_update();
  if (show) {
    show = show;
    TABBAR_HEIGHT = height + padding * 2;
    printf("nweheight");
    SetWindowPos(bar_wnd, 0,
                 cr.left, 0,
                 width, height + padding * 2,
                 SWP_NOZORDER);
    ShowWindow(bar_wnd, SW_SHOW);
  } else {
    TABBAR_HEIGHT = 0;
    prev_tab_width = 0;
    ShowWindow(bar_wnd, SW_HIDE);
  }
}

bool win_tabbar_visible()
{
  return TABBAR_HEIGHT > 0;//IsWindowVisible(bar_wnd);
}

void
win_update_tabbar()
{
  if (win_tabbar_visible()) {
    win_toggle_tabbar(true);
  }
}

void
win_open_tabbar()
{
  SendMessage(wnd, WM_USER, 0, 4);
  win_toggle_tabbar(true);
  win_adapt_term_size(false, false);
}

void
win_close_tabbar()
{
  win_toggle_tabbar(false);
  win_adapt_term_size(false, false);
}
