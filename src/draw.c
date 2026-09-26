/* ============================================
 *  ხატვა (GDI) — ისმება main.c-ში #include-ით
 *  (პროექტში არ ემატება!)
 * ============================================ */

/* ---------- ფერები ---------- */
#define COLOR_BG     RGB(23, 28, 42)
#define COLOR_BOARD  RGB(15, 19, 30)
#define COLOR_GRID   RGB(32, 39, 56)
#define COLOR_LABEL  RGB(128, 140, 168)
#define COLOR_TEXT   RGB(240, 240, 240)

/* ინდექსი 1-7: I O T S Z J L  (0 არ გამოიყენება) */
static const COLORREF piece_color[8] = { 0,
    RGB(0x40,0xC2,0xD6), RGB(0xE2,0xC0,0x44), RGB(0xA2,0x6A,0xD6), RGB(0x56,0xBE,0x6E),
    RGB(0xDA,0x54,0x5C), RGB(0x4A,0x7A,0xE0), RGB(0xE2,0x8C,0x3E) };
static const COLORREF piece_light[8] = { 0,
    RGB(0x68,0xEA,0xFE), RGB(0xFF,0xE8,0x6C), RGB(0xCA,0x92,0xFE), RGB(0x7E,0xE6,0x96),
    RGB(0xFF,0x7C,0x84), RGB(0x72,0xA2,0xFF), RGB(0xFF,0xB4,0x66) };
static const COLORREF piece_dark[8] = { 0,
    RGB(0x2E,0x8C,0x9A), RGB(0xA3,0x8A,0x31), RGB(0x75,0x4C,0x9A), RGB(0x3E,0x89,0x4F),
    RGB(0x9D,0x3C,0x42), RGB(0x35,0x58,0xA1), RGB(0xA3,0x65,0x2D) };

static HBRUSH br_bg, br_board, br_grid;
static HBRUSH br_cell[8], br_light[8], br_dark[8];
static HFONT  font_main, font_small;

static HFONT make_font(int height, int weight)
{
    return CreateFontW(-height, 0, 0, 0, weight, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
}

void draw_init(void)
{
    br_bg    = CreateSolidBrush(COLOR_BG);
    br_board = CreateSolidBrush(COLOR_BOARD);
    br_grid  = CreateSolidBrush(COLOR_GRID);
    for (int i = 1; i <= 7; i++) {
        br_cell[i]  = CreateSolidBrush(piece_color[i]);
        br_light[i] = CreateSolidBrush(piece_light[i]);
        br_dark[i]  = CreateSolidBrush(piece_dark[i]);
    }
    font_main  = make_font(22, FW_BOLD);
    font_small = make_font(14, FW_NORMAL);
}

void draw_free(void)
{
    DeleteObject(br_bg);
    DeleteObject(br_board);
    DeleteObject(br_grid);
    for (int i = 1; i <= 7; i++) {
        DeleteObject(br_cell[i]);
        DeleteObject(br_light[i]);
        DeleteObject(br_dark[i]);
    }
    DeleteObject(font_main);
    DeleteObject(font_small);
}

/* ---------- პატარა დამხმარეები ---------- */

static void fill(HDC dc, int x, int y, int w, int h, HBRUSH brush)
{
    RECT r = { x, y, x + w, y + h };
    FillRect(dc, &r, brush);
}

static void text(HDC dc, int x, int y, const wchar_t *s)
{
    TextOutW(dc, x, y, s, lstrlenW(s));
}

static void number(HDC dc, int x, int y, int value)
{
    wchar_t buf[16];
    wsprintfW(buf, L"%d", value);
    text(dc, x, y, buf);
}

static void set_text(HDC dc, HFONT font, COLORREF color)
{
    SelectObject(dc, font);
    SetTextColor(dc, color);
}

/* ---------- ბლოკები ---------- */

static void draw_cell(HDC dc, int x, int y, int color)
{
    fill(dc, x + 1, y + 1,        CELL - 2, CELL - 2, br_cell[color]);
    fill(dc, x + 1, y + 1,        CELL - 2, 4,        br_light[color]);
    fill(dc, x + 1, y + CELL - 5, CELL - 2, 4,        br_dark[color]);
}

/* px, py — 4x4 ყუთის ზედა მარცხენა კუთხე; min_y-ზე მაღლა არ იხატება */
static void draw_piece_at(HDC dc, int type, int rot, int px, int py, int min_y)
{
    for (int c = 0; c < 4; c++) {
        int x = px + PIECES[type][rot][c][0] * CELL;
        int y = py + PIECES[type][rot][c][1] * CELL;
        if (y >= min_y)
            draw_cell(dc, x, y, type + 1);
    }
}

/* ---------- ბადე ---------- */

static void draw_board(HDC dc)
{
    fill(dc, BOARD_X, BOARD_Y, BOARD_W, BOARD_H, br_board);

    for (int i = 0; i <= COLS; i++)
        fill(dc, BOARD_X + i * CELL, BOARD_Y, 1, BOARD_H, br_grid);
    for (int i = 0; i <= ROWS; i++)
        fill(dc, BOARD_X, BOARD_Y + i * CELL, BOARD_W, 1, br_grid);

    for (int row = HIDDEN; row < TOTAL_ROWS; row++)
        for (int col = 0; col < COLS; col++)
            if (g.board[row][col])
                draw_cell(dc, BOARD_X + col * CELL,
                              BOARD_Y + (row - HIDDEN) * CELL, g.board[row][col]);

    draw_piece_at(dc, g.cur_type, g.cur_rot,
                  BOARD_X + g.cur_x * CELL,
                  BOARD_Y + (g.cur_y - HIDDEN) * CELL,
                  BOARD_Y);
}

/* ---------- გვერდითი პანელი ---------- */

static void draw_panel(HDC dc)
{
    wchar_t buf[32];

    /* NEXT */
    fill(dc, PANEL_X, NEXT_Y, 150, 90, br_board);
    draw_piece_at(dc, g.next_type, 0, PANEL_X + 15, NEXT_Y + 15, 0);

    /* წარწერები */
    set_text(dc, font_main, COLOR_LABEL);
    text(dc, PANEL_X,       BOARD_Y - 2, L"NEXT");
    text(dc, PANEL_X + 165, BOARD_Y - 2, L"TIME");
    text(dc, PANEL_X, 150, L"SCORE");
    text(dc, PANEL_X, 225, L"LEVEL");
    text(dc, PANEL_X, 300, L"LINES");
    text(dc, PANEL_X, SLOT_Y - 28, L"SLOTS");

    /* მნიშვნელობები */
    set_text(dc, font_main, COLOR_TEXT);
    wsprintfW(buf, L"%02d:%02d", g.game_time / 60, g.game_time % 60);
    text(dc, PANEL_X + 165, NEXT_Y + 2, buf);
    number(dc, PANEL_X, 176, g.score);
    number(dc, PANEL_X, 251, g.level);
    number(dc, PANEL_X, 326, g.lines);

    /* სლოტები */
    set_text(dc, font_small, COLOR_TEXT);
    for (int i = 0; i < SLOT_COUNT; i++) {
        if (slot_score[i] < 0)
            wsprintfW(buf, L"%d:   -", i + 1);
        else
            wsprintfW(buf, L"%d:   %d", i + 1, slot_score[i]);
        text(dc, PANEL_X, SLOT_Y + i * 22, buf);
    }

    /* მართვა */
    set_text(dc, font_small, COLOR_LABEL);
    text(dc, PANEL_X, CLIENT_H - 100, L"Left / Right   move");
    text(dc, PANEL_X, CLIENT_H - 80,  L"Up   rotate    Down   soft drop");
    text(dc, PANEL_X, CLIENT_H - 60,  L"Space  drop    P  pause    N  new");
    text(dc, PANEL_X, CLIENT_H - 40,  L"Ctrl+1..3 save    1..3 load");
}

/* ---------- PAUSED / GAME OVER ---------- */

static void draw_overlay(HDC dc)
{
    const wchar_t *title, *hint;
    if (g.game_over) {
        title = L"GAME OVER";
        hint  = L"Enter - new game";
    } else if (g.paused) {
        title = L"PAUSED";
        hint  = L"P - continue";
    } else {
        return;
    }

    int mid = BOARD_Y + BOARD_H / 2;
    fill(dc, BOARD_X, mid - 40, BOARD_W, 80, br_bg);

    RECT r = { BOARD_X, mid - 36, BOARD_X + BOARD_W, mid + 4 };
    set_text(dc, font_main, COLOR_TEXT);
    DrawTextW(dc, title, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    RECT r2 = { BOARD_X, mid + 4, BOARD_X + BOARD_W, mid + 36 };
    set_text(dc, font_small, COLOR_LABEL);
    DrawTextW(dc, hint, -1, &r2, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

/* ---------- მთელი სცენა ---------- */

void draw_scene(HDC dc)
{
    SetBkMode(dc, TRANSPARENT);
    fill(dc, 0, 0, CLIENT_W, CLIENT_H, br_bg);
    draw_board(dc);
    draw_panel(dc);
    draw_overlay(dc);
}
