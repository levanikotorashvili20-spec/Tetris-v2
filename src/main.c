/* ============================================
 *  ფანჯარა, შეტყობინებები, ტაიმერები, კლავიატურა
 *
 *  პროექტში მხოლოდ ეს ფაილი (და tetris.rc) ემატება,
 *  დანარჩენი #include-ით ისმება — ერთი კომპილაცია
 * ============================================ */
#define _CRT_SECURE_NO_WARNINGS     /* MSVC: fopen/sprintf-ის გაფრთხილების გამორთვა */
#include <windows.h>

#include "config.h"     /* ზომები და მუდმივები */
#include "game.c"       /* თამაშის ლოგიკა, შენახვა */
#include "draw.c"       /* ხატვა */

static int  timer_level = -1;   /* რომელი დონისთვისაა ტაიმერი დაყენებული */
static SIZE min_window;         /* ფანჯრის მინიმალური ზომა */

/* გრავიტაციის ტაიმერი მიმდინარე დონის სიჩქარეზე */
static void sync_timer(HWND hwnd)
{
    if (g.level != timer_level) {
        SetTimer(hwnd, TIMER_ID, gravity_interval_ms(), NULL);  /* იგივე ID ცვლის ძველს */
        timer_level = g.level;
    }
}

/* ---------- კლავიატურა ---------- */

static void on_key(HWND hwnd, WPARAM key)
{
    if (key == VK_ESCAPE) {
        DestroyWindow(hwnd);
        return;
    }

    /* 1,2,3 = ჩატვირთვა;  Ctrl+1,2,3 = შენახვა */
    if (key >= '1' && key <= '3') {
        int slot = (int)(key - '1');
        if (GetKeyState(VK_CONTROL) < 0)
            save_slot(slot);
        else
            load_slot(slot);
    }
    else if (key == 'N') {
        new_game();
    }
    else if (g.game_over) {
        if (key == VK_RETURN)
            new_game();
    }
    else if (key == 'P') {
        g.paused = !g.paused;
    }
    else if (!g.paused) {
        switch (key) {
        case VK_LEFT:  try_move(-1, 0, 0); break;
        case VK_RIGHT: try_move(1, 0, 0);  break;
        case VK_UP:    rotate_piece();     break;
        case VK_DOWN:  if (step_down()) g.score++; break;   /* 1 ქულა რიგზე */
        case VK_SPACE: hard_drop();        break;
        default: return;                   /* სხვა ღილაკზე გადახატვა არ სჭირდება */
        }
    }

    sync_timer(hwnd);
    InvalidateRect(hwnd, NULL, FALSE);
}

/* ---------- ტაიმერები ---------- */

static void on_timer(HWND hwnd, WPARAM id)
{
    if (g.game_over || g.paused)
        return;

    if (id == CLOCK_ID)
        g.game_time++;
    else
        step_down();

    sync_timer(hwnd);
    InvalidateRect(hwnd, NULL, FALSE);
}

/* ---------- ხატვა: ჯერ მეხსიერებაში, მერე ეკრანზე ---------- */

static void on_paint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC screen = BeginPaint(hwnd, &ps);

    RECT rc;
    GetClientRect(hwnd, &rc);
    int cw = rc.right, ch = rc.bottom;

    if (cw > 0 && ch > 0) {
        HDC mem = CreateCompatibleDC(screen);
        HBITMAP bmp = CreateCompatibleBitmap(screen, cw, ch);
        HGDIOBJ old = SelectObject(mem, bmp);

        HBRUSH bg = CreateSolidBrush(RGB(23, 28, 42));
        FillRect(mem, &rc, bg);             /* გვერდითი ზოლები */
        DeleteObject(bg);

        /* მასშტაბი: პროპორციის შენარჩუნებით, ცენტრში */
        int vw, vh;
        if (cw * CLIENT_H <= ch * CLIENT_W) {
            vw = cw;
            vh = cw * CLIENT_H / CLIENT_W;
        } else {
            vh = ch;
            vw = ch * CLIENT_W / CLIENT_H;
        }

        SaveDC(mem);
        SetMapMode(mem, MM_ANISOTROPIC);
        SetWindowExtEx(mem, CLIENT_W, CLIENT_H, NULL);
        SetViewportExtEx(mem, vw, vh, NULL);
        SetViewportOrgEx(mem, (cw - vw) / 2, (ch - vh) / 2, NULL);

        draw_scene(mem);

        RestoreDC(mem, -1);                 /* ისევ 1:1 BitBlt-ისთვის */
        BitBlt(screen, 0, 0, cw, ch, mem, 0, 0, SRCCOPY);

        SelectObject(mem, old);
        DeleteObject(bmp);
        DeleteDC(mem);
    }

    EndPaint(hwnd, &ps);
}

/* ---------- WndProc ---------- */

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_PAINT:
        on_paint(hwnd);
        return 0;

    case WM_ERASEBKGND:
        return 1;                           /* ციმციმის გარეშე */

    case WM_KEYDOWN:
        on_key(hwnd, wp);
        return 0;

    case WM_TIMER:
        on_timer(hwnd, wp);
        return 0;

    case WM_SIZE:
        if (wp == SIZE_MINIMIZED && !g.game_over)
            g.paused = 1;                   /* ჩაკეცვისას პაუზა */
        return 0;

    case WM_GETMINMAXINFO: {
        MINMAXINFO *mm = (MINMAXINFO *)lp;
        mm->ptMinTrackSize.x = min_window.cx;
        mm->ptMinTrackSize.y = min_window.cy;
        return 0;
    }

    case WM_DESTROY:
        KillTimer(hwnd, TIMER_ID);
        KillTimer(hwnd, CLOCK_ID);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

/* ---------- შესვლის წერტილი ---------- */

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE prev, LPSTR cmd, int show)
{
    (void)prev; (void)cmd;

    SetProcessDPIAware();
    draw_init();
    init_slots();
    game_init(GetTickCount());

    WNDCLASSEXW wc = { 0 };
    wc.cbSize        = sizeof wc;
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hinst;
    wc.hIcon         = LoadIconW(hinst, MAKEINTRESOURCEW(1));   /* tetris.rc */
    wc.hIconSm       = wc.hIcon;
    wc.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.lpszClassName = L"TetrisCWnd";
    RegisterClassExW(&wc);

    RECT r = { 0, 0, CLIENT_W, CLIENT_H };
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    min_window.cx = r.right - r.left;
    min_window.cy = r.bottom - r.top;

    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"Tetris",
                                WS_OVERLAPPEDWINDOW,
                                120, 80, min_window.cx, min_window.cy,
                                NULL, NULL, hinst, NULL);

    sync_timer(hwnd);
    SetTimer(hwnd, CLOCK_ID, 1000, NULL);

    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    draw_free();
    return 0;
}
