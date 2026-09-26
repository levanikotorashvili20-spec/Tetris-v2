/* ============================================
 *  განლაგება და ზოგადი მუდმივები
 * ============================================ */
#pragma once

#define CELL        30              /* უჯრის ზომა პიქსელებში */
#define COLS        10              /* ბადის სვეტები */
#define ROWS        20              /* ხილული რიგები */
#define HIDDEN      2               /* დამალული რიგები ზემოთ (ფიგურის დაბადება) */
#define TOTAL_ROWS  (ROWS + HIDDEN)

#define MARGIN      14
#define PANEL_W     260

#define BOARD_X     MARGIN
#define BOARD_Y     MARGIN
#define BOARD_W     (COLS * CELL)
#define BOARD_H     (ROWS * CELL)

#define CLIENT_W    (MARGIN * 2 + BOARD_W + PANEL_W)
#define CLIENT_H    (MARGIN * 2 + BOARD_H)

#define PANEL_X     (BOARD_X + BOARD_W + 20)
#define NEXT_Y      (BOARD_Y + 26)  /* NEXT ველის ზედა კიდე */
#define SLOT_Y      403             /* სლოტების სიის დასაწყისი */

#define SLOT_COUNT  3

#define TIMER_ID    1               /* გრავიტაცია */
#define CLOCK_ID    2               /* საათი: ყოველ წამში */
