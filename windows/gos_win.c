#define UNICODE
#define _UNICODE
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <psapi.h>

#define APP_NAME L"GOS for Windows"
#define TIMER_ID 1
#define TIMER_MS 250
#define WM_GOS_COMMAND (WM_APP + 1)

static HWND mainWnd, terminalEdit, statusText;
static HFONT uiFont, monoFont;
static int dark = 1;

static void set_text(HWND h, const wchar_t *s) { SetWindowTextW(h, s); }

static void append_terminal(const wchar_t *s) {
    int n = GetWindowTextLengthW(terminalEdit);
    SendMessageW(terminalEdit, EM_SETSEL, n, n);
    SendMessageW(terminalEdit, EM_REPLACESEL, FALSE, (LPARAM)s);
    SendMessageW(terminalEdit, EM_SCROLLCARET, 0, 0);
}

static void cmd(const wchar_t *command) {
    wchar_t out[4096];
    if (!_wcsicmp(command, L"help")) {
        wcscpy(out, L"\r\nGOS commands:\r\n  help       show commands\r\n  clear      clear console\r\n  version    show GOS version\r\n  time       show local time\r\n  mem        show memory usage\r\n  dir        list current directory\r\n  open       open File Explorer\r\n  shutdown   close GOS\r\n\r\nGOS> ");
    } else if (!_wcsicmp(command, L"clear")) {
        SetWindowTextW(terminalEdit, L"GOS Terminal\r\n\r\nGOS> ");
        return;
    } else if (!_wcsicmp(command, L"version")) {
        wcscpy(out, L"\r\nGOS Windows Edition 1.0\r\nNative Win32 / low-resource mode\r\nGOS> ");
    } else if (!_wcsicmp(command, L"time")) {
        SYSTEMTIME t; GetLocalTime(&t);
        swprintf(out, 4096, L"\r\n%02u:%02u:%02u  %02u/%02u/%04u\r\nGOS> ",
                 t.wHour,t.wMinute,t.wSecond,t.wDay,t.wMonth,t.wYear);
    } else if (!_wcsicmp(command, L"mem")) {
        MEMORYSTATUSEX m; m.dwLength=sizeof(m);
        GlobalMemoryStatusEx(&m);
        swprintf(out, 4096, L"\r\nMemory: %llu MB total, %llu MB available, %lu%% used\r\nGOS> ",
                 (unsigned long long)(m.ullTotalPhys/1048576),
                 (unsigned long long)(m.ullAvailPhys/1048576),
                 (unsigned long)(m.dwMemoryLoad));
    } else if (!_wcsicmp(command, L"dir")) {
        WIN32_FIND_DATAW fd; HANDLE h=FindFirstFileW(L"*",&fd);
        wcscpy(out,L"\r\n");
        if(h!=INVALID_HANDLE_VALUE) {
            do {
                if(wcslen(out)>3800) break;
                wcscat(out, fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY?L"[DIR] ":L"      ");
                wcscat(out,fd.cFileName); wcscat(out,L"\r\n");
            } while(FindNextFileW(h,&fd));
            FindClose(h);
        } else wcscat(out,L"(cannot read directory)\r\n");
        wcscat(out,L"GOS> ");
    } else if (!_wcsicmp(command, L"open")) {
        ShellExecuteW(mainWnd,L"open",L"explorer.exe",NULL,NULL,SW_SHOWNORMAL);
        return;
    } else if (!_wcsicmp(command, L"shutdown")) {
        PostMessageW(mainWnd,WM_CLOSE,0,0); return;
    } else {
        swprintf(out,4096,L"\r\nUnknown command: %ls\r\nType help for commands.\r\nGOS> ",command);
    }
    append_terminal(out);
}

static void terminal_key(HWND h, wchar_t key) {
    if (key==L'\r') {
        int len=GetWindowTextLengthW(h);
        wchar_t *buf=(wchar_t*)calloc((size_t)len+1,sizeof(wchar_t));
        if(!buf)return;
        GetWindowTextW(h,buf,len+1);
        wchar_t *p=wcsrchr(buf,L'>');
        if(p) { p++; while(*p==L' ')p++; cmd(p); }
        free(buf);
    }
}

static LRESULT CALLBACK TerminalProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if(m==WM_CHAR && w==L'\r') { terminal_key(h,(wchar_t)w); return 0; }
    return CallWindowProcW((WNDPROC)GetPropW(h,L"GOS_OLDPROC"),h,m,w,l);
}

static void draw_card(HDC dc, RECT r, const wchar_t *title, COLORREF accent) {
    HBRUSH b=CreateSolidBrush(RGB(28,33,48)); FillRect(dc,&r,b); DeleteObject(b);
    HPEN p=CreatePen(PS_SOLID,1,RGB(70,78,100)); HPEN old=(HPEN)SelectObject(dc,p);
    Rectangle(dc,r.left,r.top,r.right,r.bottom); SelectObject(dc,old); DeleteObject(p);
    SetTextColor(dc,RGB(235,240,250)); SetBkMode(dc,TRANSPARENT);
    RECT t={r.left+16,r.top+12,r.right-12,r.top+42}; SelectObject(dc,uiFont);
    DrawTextW(dc,title,-1,&t,DT_LEFT|DT_SINGLELINE);
    HBRUSH a=CreateSolidBrush(accent); RECT bar={r.left+2,r.top+2,r.right-2,r.top+5}; FillRect(dc,&bar,a); DeleteObject(a);
}

static void paint_dashboard(HWND h,HDC dc) {
    RECT c; GetClientRect(h,&c);
    HBRUSH bg=CreateSolidBrush(RGB(13,17,27)); FillRect(dc,&c,bg); DeleteObject(bg);
    SetBkMode(dc,TRANSPARENT); SelectObject(dc,uiFont);
    SetTextColor(dc,RGB(110,190,255)); TextOutW(dc,24,18,L"GOS",3);
    SetTextColor(dc,RGB(235,240,250)); TextOutW(dc,82,18,L"DESKTOP",7);

    RECT r={24,65,390,235}; draw_card(dc,r,L"SYSTEM",RGB(50,125,210));
    SetTextColor(dc,RGB(130,230,175)); TextOutW(dc,48,112,L"READY",5);
    SetTextColor(dc,RGB(185,195,215)); TextOutW(dc,48,145,L"Native Windows edition",22);
    TextOutW(dc,48,173,L"Low-resource desktop",20);
    TextOutW(dc,48,201,L"Keyboard + terminal online",25);

    RECT r2={420,65,760,235}; draw_card(dc,r2,L"PERFORMANCE",RGB(115,80,190));
    MEMORYSTATUSEX m; m.dwLength=sizeof(m); GlobalMemoryStatusEx(&m);
    int used=(int)m.dwMemoryLoad;
    SetTextColor(dc,RGB(220,225,240)); TextOutW(dc,444,112,L"MEMORY",6);
    RECT bar={444,140,720,156}; HBRUSH b=CreateSolidBrush(RGB(50,55,72));FillRect(dc,&bar,b);DeleteObject(b);
    bar.right=bar.left+(bar.right-bar.left)*used/100;
    b=CreateSolidBrush(RGB(100,205,160));FillRect(dc,&bar,b);DeleteObject(b);
    wchar_t s[128]; swprintf(s,128,L"%d%% used",used); TextOutW(dc,444,165,s,(int)wcslen(s));

    FILETIME a,bf,cft,d; ULARGE_INTEGER u1,u2;
    if(GetSystemTimes(&a,&bf,&cft)) {
        u1.LowPart=a.dwLowDateTime;u1.HighPart=a.dwHighDateTime;
        u2.LowPart=cft.dwLowDateTime;u2.HighPart=cft.dwHighDateTime;
        (void)u1;(void)u2;
    }
    SetTextColor(dc,RGB(170,185,210)); TextOutW(dc,444,195,L"Native Win32 / no runtime required",34);

    RECT foot={0,c.bottom-44,c.right,c.bottom};
    b=CreateSolidBrush(RGB(20,24,38));FillRect(dc,&foot,b);DeleteObject(b);
    SetTextColor(dc,RGB(170,185,210)); TextOutW(dc,20,c.bottom-30,L"F1 Terminal    F2 Explorer    F10 Exit",36);
}

static void create_terminal(void) {
    terminalEdit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"GOS Terminal\r\n\r\nGOS> ",
        WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN|WS_VSCROLL,
        24,270,736,260,mainWnd,(HMENU)100,(HINSTANCE)GetWindowLongPtrW(mainWnd,GWLP_HINSTANCE),NULL);
    SendMessageW(terminalEdit,WM_SETFONT,(WPARAM)monoFont,TRUE);
    SendMessageW(terminalEdit,EM_SETREADONLY,FALSE,0);
}

static LRESULT CALLBACK WndProc(HWND h,UINT m,WPARAM w,LPARAM l) {
    if(m==WM_CREATE) {
        uiFont=CreateFontW(20,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,L"Segoe UI");
        monoFont=CreateFontW(17,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,FIXED_PITCH|FF_MODERN,L"Consolas");
        SetTimer(h,TIMER_ID,TIMER_MS,NULL);
        return 0;
    }
    if(m==WM_TIMER) { InvalidateRect(h,NULL,FALSE); return 0; }
    if(m==WM_KEYDOWN) {
        if(w==VK_F10){PostMessageW(h,WM_CLOSE,0,0);return 0;}
        if(w==VK_F1){if(!terminalEdit)create_terminal();SetFocus(terminalEdit);return 0;}
        if(w==VK_F2){ShellExecuteW(h,L"open",L"explorer.exe",NULL,NULL,SW_SHOWNORMAL);return 0;}
    }
    if(m==WM_PAINT){PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);paint_dashboard(h,dc);EndPaint(h,&ps);return 0;}
    if(m==WM_DESTROY){KillTimer(h,TIMER_ID);if(uiFont)DeleteObject(uiFont);if(monoFont)DeleteObject(monoFont);PostQuitMessage(0);return 0;}
    return DefWindowProcW(h,m,w,l);
}

int WINAPI wWinMain(HINSTANCE inst,HINSTANCE prev,PWSTR cmdline,int show) {
    (void)prev;(void)cmdline;
    WNDCLASSEXW wc={sizeof(wc)}; wc.lpfnWndProc=WndProc; wc.hInstance=inst; wc.lpszClassName=L"GOSWinClass";
    wc.hCursor=LoadCursor(NULL,IDC_ARROW); wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
    RegisterClassExW(&wc);
    mainWnd=CreateWindowExW(0,L"GOSWinClass",APP_NAME,WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
        CW_USEDEFAULT,CW_USEDEFAULT,800,600,NULL,NULL,inst,NULL);
    if(!mainWnd)return 1;
    ShowWindow(mainWnd,show);UpdateWindow(mainWnd);
    MSG msg;while(GetMessageW(&msg,NULL,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}
    return (int)msg.wParam;
}
