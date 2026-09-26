/* ============================================
 *  თამაშის ლოგიკა — ისმება main.c-ში #include-ით
 *  (პროექტში არ ემატება!)
 *  Windows-ზე არ არის დამოკიდებული
 * ============================================ */
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* თამაშის მთელი მდგომარეობა ერთ სტრუქტურაში —
   შენახვა არის ამ სტრუქტურის ფაილში ჩაწერა */
typedef struct {
    uint32_t magic;             /* ფაილის შემოწმებისთვის */
    int32_t  cur_type;          /* მიმდინარე ფიგურა 0-6 */
    int32_t  cur_rot;           /* როტაცია 0-3 */
    int32_t  cur_x;             /* 4x4 ყუთის სვეტი (შეიძლება უარყოფითი) */
    int32_t  cur_y;             /* 4x4 ყუთის რიგი (დამალულების ჩათვლით) */
    int32_t  next_type;
    int32_t  score;
    int32_t  lines;
    int32_t  level;
    int32_t  game_time;         /* წამებში */
    int32_t  game_over;
    int32_t  paused;
    uint32_t seed;
    int32_t  bag_pos;
    uint8_t  bag[7];            /* 7-bag */
    uint8_t  board[TOTAL_ROWS][COLS];   /* 0 = ცარიელი, 1-7 = ფერი */
} GameState;


#define SAVE_MAGIC 0x31435454u      /* "TTC1" */

GameState g;
int slot_score[SLOT_COUNT] = { -1, -1, -1 };

/* 4x4 ყუთში, (0,0) ზედა მარცხენა.  რიგი: I O T S Z J L */
const int8_t PIECES[7][4][4][2] = {
    { {{0,1},{1,1},{2,1},{3,1}}, {{2,0},{2,1},{2,2},{2,3}},     /* I */
      {{0,2},{1,2},{2,2},{3,2}}, {{1,0},{1,1},{1,2},{1,3}} },
    { {{1,0},{2,0},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{2,1}},     /* O */
      {{1,0},{2,0},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{2,1}} },
    { {{1,0},{0,1},{1,1},{2,1}}, {{1,0},{1,1},{2,1},{1,2}},     /* T */
      {{0,1},{1,1},{2,1},{1,2}}, {{1,0},{0,1},{1,1},{1,2}} },
    { {{1,0},{2,0},{0,1},{1,1}}, {{1,0},{1,1},{2,1},{2,2}},     /* S */
      {{1,1},{2,1},{0,2},{1,2}}, {{0,0},{0,1},{1,1},{1,2}} },
    { {{0,0},{1,0},{1,1},{2,1}}, {{2,0},{1,1},{2,1},{1,2}},     /* Z */
      {{0,1},{1,1},{1,2},{2,2}}, {{1,0},{0,1},{1,1},{0,2}} },
    { {{0,0},{0,1},{1,1},{2,1}}, {{1,0},{2,0},{1,1},{1,2}},     /* J */
      {{0,1},{1,1},{2,1},{2,2}}, {{1,0},{1,1},{0,2},{1,2}} },
    { {{2,0},{0,1},{1,1},{2,1}}, {{1,0},{1,1},{1,2},{2,2}},     /* L */
      {{0,1},{1,1},{2,1},{0,2}}, {{0,0},{1,0},{1,1},{1,2}} },
};

/* ---------- შემთხვევითობა ---------- */

static int rnd(void)                    /* 0..32767 */
{
    g.seed = g.seed * 1103515245u + 12345u;
    return (int)((g.seed >> 16) & 0x7FFF);
}

static int next_from_bag(void)
{
    if (g.bag_pos >= 7) {
        for (int i = 0; i < 7; i++)
            g.bag[i] = (uint8_t)i;
        for (int i = 6; i > 0; i--) {   /* Fisher-Yates */
            int j = rnd() % (i + 1);
            uint8_t t = g.bag[i];
            g.bag[i] = g.bag[j];
            g.bag[j] = t;
        }
        g.bag_pos = 0;
    }
    return g.bag[g.bag_pos++];
}

/* ---------- ძირითადი წესები ---------- */

int can_place(int type, int rot, int x, int y)
{
    for (int c = 0; c < 4; c++) {
        int cx = x + PIECES[type][rot][c][0];
        int cy = y + PIECES[type][rot][c][1];
        if (cx < 0 || cx >= COLS || cy < 0 || cy >= TOTAL_ROWS)
            return 0;
        if (g.board[cy][cx] != 0)
            return 0;
    }
    return 1;
}

int try_move(int dx, int dy, int drot)
{
    int nx = g.cur_x + dx;
    int ny = g.cur_y + dy;
    int nr = (g.cur_rot + drot) & 3;
    if (!can_place(g.cur_type, nr, nx, ny))
        return 0;
    g.cur_x = nx;
    g.cur_y = ny;
    g.cur_rot = nr;
    return 1;
}

void rotate_piece(void)
{
    static const int kicks[] = { 0, -1, 1, -2, 2 };   /* კედლიდან აცილება */
    for (int i = 0; i < 5; i++)
        if (try_move(kicks[i], 0, 1))
            return;
}

static void lock_piece(void)
{
    for (int c = 0; c < 4; c++) {
        int cx = g.cur_x + PIECES[g.cur_type][g.cur_rot][c][0];
        int cy = g.cur_y + PIECES[g.cur_type][g.cur_rot][c][1];
        g.board[cy][cx] = (uint8_t)(g.cur_type + 1);
    }
}

static int clear_lines(void)
{
    int cleared = 0;
    int row = TOTAL_ROWS - 1;
    while (row >= 0) {
        int full = 1;
        for (int col = 0; col < COLS; col++)
            if (g.board[row][col] == 0) { full = 0; break; }

        if (full) {
            /* ზემოთა რიგები ერთით ქვემოთ, ზედა რიგი ცარიელდება */
            memmove(g.board[1], g.board[0], (size_t)row * COLS);
            memset(g.board[0], 0, COLS);
            cleared++;                  /* იგივე რიგს თავიდან ვამოწმებთ */
        } else {
            row--;
        }
    }
    return cleared;
}

static void add_score(int cleared)
{
    static const int table[] = { 0, 40, 100, 300, 1200 };
    if (cleared == 0)
        return;
    g.lines += cleared;
    g.score += table[cleared] * (g.level + 1);
    g.level = g.lines / 10;
}

static void spawn_piece(void)
{
    g.cur_type = g.next_type;
    g.next_type = next_from_bag();
    g.cur_rot = 0;
    g.cur_x = 3;
    g.cur_y = 1;
    if (!can_place(g.cur_type, g.cur_rot, g.cur_x, g.cur_y))
        g.game_over = 1;
}

/* ერთით ქვემოთ; თუ ვერ — ჩაკეტვა. აბრუნებს 1-ს, თუ ჩამოვიდა */
int step_down(void)
{
    if (try_move(0, 1, 0))
        return 1;
    lock_piece();
    add_score(clear_lines());
    spawn_piece();
    return 0;
}

void hard_drop(void)
{
    while (try_move(0, 1, 0))
        g.score += 2;
    step_down();
}

int gravity_interval_ms(void)
{
    int ms = 800 - g.level * 70;
    return ms < 100 ? 100 : ms;
}

void new_game(void)
{
    uint32_t seed = g.seed;             /* შემთხვევითობა გრძელდება */
    memset(&g, 0, sizeof g);
    g.magic = SAVE_MAGIC;
    g.seed = seed;
    g.bag_pos = 7;
    g.next_type = next_from_bag();
    spawn_piece();
}

void game_init(uint32_t seed)
{
    g.seed = seed;
    new_game();
}

/* ---------- შენახვა / ჩატვირთვა ---------- */

static void slot_file_name(int slot, char *out)
{
    sprintf(out, "slot%d.sav", slot + 1);
}

/* ამოწმებს, რომ ფაილიდან წაკითხული მონაცემი თამაშს არ გააფუჭებს */
static int state_is_valid(const GameState *s)
{
    if (s->magic != SAVE_MAGIC) return 0;
    if (s->cur_type < 0 || s->cur_type > 6) return 0;
    if (s->next_type < 0 || s->next_type > 6) return 0;
    if (s->cur_rot < 0 || s->cur_rot > 3) return 0;
    if (s->bag_pos < 0 || s->bag_pos > 7) return 0;
    for (int i = 0; i < 7; i++)
        if (s->bag[i] > 6) return 0;
    for (int r = 0; r < TOTAL_ROWS; r++)
        for (int c = 0; c < COLS; c++)
            if (s->board[r][c] > 7) return 0;
    return 1;
}

static int read_slot(int slot, GameState *out)
{
    char name[32];
    slot_file_name(slot, name);
    FILE *f = fopen(name, "rb");
    if (!f)
        return 0;
    size_t n = fread(out, sizeof *out, 1, f);
    fclose(f);
    return n == 1 && state_is_valid(out);
}

int save_slot(int slot)
{
    if (g.game_over)                    /* დასრულებულ თამაშს არ ვინახავთ */
        return 0;
    char name[32];
    slot_file_name(slot, name);
    FILE *f = fopen(name, "wb");
    if (!f)
        return 0;
    size_t n = fwrite(&g, sizeof g, 1, f);
    fclose(f);
    if (n != 1)
        return 0;
    slot_score[slot] = g.score;
    return 1;
}

int load_slot(int slot)
{
    GameState tmp;
    if (!read_slot(slot, &tmp))
        return 0;
    g = tmp;
    g.paused = 1;                       /* ჩატვირთული თამაში პაუზაზე იწყება */
    return 1;
}

void init_slots(void)
{
    GameState tmp;
    for (int i = 0; i < SLOT_COUNT; i++)
        slot_score[i] = read_slot(i, &tmp) ? tmp.score : -1;
}
