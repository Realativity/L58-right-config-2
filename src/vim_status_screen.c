/*
 * Custom ZMK status screen for a 128x32 OLED (Lily58).
 *
 *  Row 1:     active layer (left)         WPM (right)
 *  Rows 2-4:  vim cheat sheet, rotating every PAGE_INTERVAL_MS
 *
 * Font: unscii_8 (8x8 px, fixed width) -> 16 characters x 4 rows.
 * Keep every cheat-sheet line at 16 characters or fewer, 3 lines per page.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/display/status_screen.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/wpm_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/wpm.h>

#define PAGE_INTERVAL_MS 4000

/* Must match the layer order in lily58.keymap */
static const char *layer_names[] = {"BASE", "MIRROR", "RAISE", "SYS"};

/* Each page: 3 lines, max 16 chars per line ("^" means Ctrl) */
static const char *vim_pages[] = {
    "h j k l   move\n"
    "w / b     word\n"
    "0 / $  line s/e",

    "gg / G  top/end\n"
    "^d / ^u  half pg\n"
    "%   match brkt",

    "i / a  ins/app\n"
    "o / O  new line\n"
    "x   delete char",

    "dd  delete line\n"
    "yy  yank line\n"
    "p / P  paste",

    "u / ^r undo/redo\n"
    "ciw  change word\n"
    ".   repeat",

    "/pat  search\n"
    "n / N  next/prev\n"
    "*   search word",

    ":w   save\n"
    ":q   quit\n"
    ":wq  save+quit",
};

#define NUM_PAGES ARRAY_SIZE(vim_pages)

static lv_obj_t *layer_label;
static lv_obj_t *wpm_label;
static lv_obj_t *vim_label;
static size_t current_page;

/* ---------- Layer name ---------- */

struct vim_layer_state {
    uint8_t index;
};

static struct vim_layer_state vim_layer_get_state(const zmk_event_t *eh) {
    return (struct vim_layer_state){.index = zmk_keymap_highest_layer_active()};
}

static void vim_layer_update_cb(struct vim_layer_state state) {
    if (layer_label == NULL) {
        return;
    }
    if (state.index < ARRAY_SIZE(layer_names)) {
        lv_label_set_text(layer_label, layer_names[state.index]);
    } else {
        char text[8];
        snprintf(text, sizeof(text), "L%u", state.index);
        lv_label_set_text(layer_label, text);
    }
}

ZMK_DISPLAY_WIDGET_LISTENER(vim_layer, struct vim_layer_state, vim_layer_update_cb,
                            vim_layer_get_state)
ZMK_SUBSCRIPTION(vim_layer, zmk_layer_state_changed);

/* ---------- Words per minute ---------- */

struct vim_wpm_state {
    uint8_t wpm;
};

static struct vim_wpm_state vim_wpm_get_state(const zmk_event_t *eh) {
    return (struct vim_wpm_state){.wpm = zmk_wpm_get_state()};
}

static void vim_wpm_update_cb(struct vim_wpm_state state) {
    if (wpm_label == NULL) {
        return;
    }
    char text[12];
    snprintf(text, sizeof(text), "WPM %u", state.wpm);
    lv_label_set_text(wpm_label, text);
}

ZMK_DISPLAY_WIDGET_LISTENER(vim_wpm, struct vim_wpm_state, vim_wpm_update_cb, vim_wpm_get_state)
ZMK_SUBSCRIPTION(vim_wpm, zmk_wpm_state_changed);

/* ---------- Rotating vim cheat sheet ---------- */

static void next_page(lv_timer_t *timer) {
    current_page = (current_page + 1) % NUM_PAGES;
    lv_label_set_text(vim_label, vim_pages[current_page]);
}

/* ---------- Screen layout ---------- */

static lv_obj_t *make_label(lv_obj_t *parent, lv_align_t align, lv_coord_t y) {
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, &lv_font_unscii_8, LV_PART_MAIN);
    lv_obj_set_style_text_line_space(label, 0, LV_PART_MAIN);
    lv_obj_align(label, align, 0, y);
    return label;
}

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);

    layer_label = make_label(screen, LV_ALIGN_TOP_LEFT, 0);
    wpm_label = make_label(screen, LV_ALIGN_TOP_RIGHT, 0);
    vim_label = make_label(screen, LV_ALIGN_TOP_LEFT, 8);

    lv_label_set_text(vim_label, vim_pages[0]);

    /* Fill in the current layer and WPM right away */
    vim_layer_init();
    vim_wpm_init();

    lv_timer_create(next_page, PAGE_INTERVAL_MS, NULL);

    return screen;
}