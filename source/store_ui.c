#include "store_ui.h"
#include "pkgi_config.h"
#include "pkgi_dialog.h"
#include "pkgi_download.h"
#include "pkgi_utils.h"
#include "pkgi_style.h"
#include "pkgi.h"

static StoreState current_state = STORE_STATE_GRID;
static int topmenu_selected = 0;
static const char* topmenu_items[] = { "All", "Games", "DLCs", "Themes", "Avatars" };
static const ContentType topmenu_types[] = { 0, ContentGame, ContentDLC, ContentTheme, ContentAvatar };
static const int NUM_TOPMENU_ITEMS = 5;

static uint32_t grid_selected_x = 0;
static uint32_t grid_selected_y = 0;
static uint32_t grid_scroll_y = 0;
static const int GRID_COLS = 4;
static const int GRID_COVER_W = 260;
static const int GRID_COVER_H = 180;
static const int GRID_PADDING = 30;

void store_ui_init(void) {
    current_state = STORE_STATE_GRID;
    topmenu_selected = 0;
    grid_selected_x = 0;
    grid_selected_y = 0;
    grid_scroll_y = 0;
}

void store_ui_start(void) {
    current_state = STORE_STATE_GRID;
}

void store_ui_do_main(pkgi_input* input, Config* config) {
    // Background - Deep PlayStation Network Blue
    pkgi_draw_fill_rect(0, 0, VITA_WIDTH, VITA_HEIGHT, 0xFF0D1726);

    // Top Header Bar
    pkgi_draw_fill_rect(0, 0, VITA_WIDTH, 120, 0x88000000);
    pkgi_draw_text(60, 45, 0xFFFFFFFF, "PlayStation(R)Store");

    // Top Categories
    int start_x = 420;
    for (int i = 0; i < NUM_TOPMENU_ITEMS; i++) {
        if (i == topmenu_selected) {
            pkgi_draw_fill_rect_z(start_x - 15, 35, PKGI_FONT_Z, pkgi_text_width(topmenu_items[i]) + 30, 50, 0xFFFFFFFF);
            pkgi_draw_text_z(start_x, 45, PKGI_FONT_Z+1, 0xFF000000, topmenu_items[i]);
        } else {
            pkgi_draw_text_z(start_x, 45, PKGI_FONT_Z, (current_state == STORE_STATE_CATEGORIES) ? 0xFFCCCCCC : 0xFF777777, topmenu_items[i]);
        }
        start_x += pkgi_text_width(topmenu_items[i]) + 60;
    }

    uint32_t db_count = pkgi_db_count();

    // Input Handling
    if (input) {
        if (current_state == STORE_STATE_CATEGORIES) {
            if (input->pressed & PKGI_BUTTON_RIGHT) {
                if (topmenu_selected < NUM_TOPMENU_ITEMS - 1) {
                    topmenu_selected++;
                    config->content = topmenu_types[topmenu_selected];
                    pkgi_db_configure(NULL, config);
                    grid_selected_x = 0; grid_selected_y = 0; grid_scroll_y = 0;
                }
            }
            if (input->pressed & PKGI_BUTTON_LEFT) {
                if (topmenu_selected > 0) {
                    topmenu_selected--;
                    config->content = topmenu_types[topmenu_selected];
                    pkgi_db_configure(NULL, config);
                    grid_selected_x = 0; grid_selected_y = 0; grid_scroll_y = 0;
                }
            }
            if (input->pressed & PKGI_BUTTON_DOWN) {
                if (db_count > 0) current_state = STORE_STATE_GRID;
            }
        } else if (current_state == STORE_STATE_GRID) {
            if (input->pressed & PKGI_BUTTON_LEFT) {
                if (grid_selected_x > 0) grid_selected_x--;
            }
            if (input->pressed & PKGI_BUTTON_RIGHT) {
                if (grid_selected_x < GRID_COLS - 1 && (grid_selected_y * GRID_COLS + grid_selected_x + 1) < db_count) grid_selected_x++;
            }
            if (input->pressed & PKGI_BUTTON_DOWN) {
                if (((grid_selected_y + 1) * GRID_COLS) < db_count) grid_selected_y++;
            }
            if (input->pressed & PKGI_BUTTON_UP) {
                if (grid_selected_y > 0) grid_selected_y--;
                else current_state = STORE_STATE_CATEGORIES;
            }
        }
    }

    // Grid Scroll
    if (grid_selected_y < grid_scroll_y) grid_scroll_y = grid_selected_y;
    if (grid_selected_y > grid_scroll_y + 2) grid_scroll_y = grid_selected_y - 2;

    int grid_start_x = (VITA_WIDTH - (GRID_COLS * (GRID_COVER_W + GRID_PADDING))) / 2;
    int grid_start_y = 180;

    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < GRID_COLS; x++) {
            uint32_t item_index = (grid_scroll_y + y) * GRID_COLS + x;
            if (item_index >= db_count) continue;
            
            DbItem* item = pkgi_db_get(item_index);
            int draw_x = grid_start_x + (x * (GRID_COVER_W + GRID_PADDING));
            int draw_y = grid_start_y + (y * (GRID_COVER_H + GRID_PADDING));
            
            // 2008 Cyan Selection Glow
            if (current_state == STORE_STATE_GRID && x == grid_selected_x && (grid_scroll_y + y) == grid_selected_y) {
                pkgi_draw_fill_rect_z(draw_x - 6, draw_y - 6, PKGI_FONT_Z, GRID_COVER_W + 12, GRID_COVER_H + 12, 0xFFFFFFFF);
                pkgi_draw_fill_rect_z(draw_x - 3, draw_y - 3, PKGI_FONT_Z, GRID_COVER_W + 6, GRID_COVER_H + 6, 0xFF00DDFF);
            }
            
            // Game Box Base
            pkgi_draw_fill_rect_z(draw_x, draw_y, PKGI_FONT_Z, GRID_COVER_W, GRID_COVER_H, 0xFF182838);
            
            // Text Details
            pkgi_clip_set(draw_x + 10, draw_y, GRID_COVER_W - 20, GRID_COVER_H);
            pkgi_draw_text_ttf(draw_x + 10, draw_y + 10, PKGI_FONT_Z, 0xFFFFFFFF, item->name);
            
            char size_text[64];
            if (item->size > 1024*1024*1024) pkgi_snprintf(size_text, sizeof(size_text), "%.2f GB", (float)item->size / (1024*1024*1024));
            else pkgi_snprintf(size_text, sizeof(size_text), "%.2f MB", (float)item->size / (1024*1024));
            pkgi_draw_text_ttf(draw_x + 10, draw_y + GRID_COVER_H - 40, PKGI_FONT_Z, 0xFFAAAAAA, size_text);
            
            pkgi_clip_remove();
            
            // Trigger Details Dialog
            if (input && current_state == STORE_STATE_GRID && (input->pressed & pkgi_ok_button()) && x == grid_selected_x && (grid_scroll_y + y) == grid_selected_y) {
                input->pressed &= ~pkgi_ok_button();
                pkgi_dialog_details(item, "Game"); // Uses built-in PKGi details and download logic!
            }
        }
    }
    
    // Bottom Help Text
    pkgi_draw_fill_rect(0, VITA_HEIGHT - 60, VITA_WIDTH, 60, 0xAA000000);
    pkgi_draw_text(60, VITA_HEIGHT - 45, 0xFFFFFFFF, "Triangle: Refresh Database  |  X: Select  |  O: Back");
}
