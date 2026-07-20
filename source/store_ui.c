#include "store_ui.h"
#include "pkgi_config.h"
#include "pkgi_dialog.h"
#include "pkgi_download.h"
#include "pkgi_utils.h"
#include "pkgi_style.h"

static StoreState current_state = STORE_STATE_GRID;

// 2008 Store Layout Constants
static const int TOPBAR_HEIGHT = 100;

// Top Menu Variables
static int topmenu_selected = 0;
static const char* topmenu_items[] = {
    "Search",
    "Latest",
    "PS3 Games",
    "PS1 Classics",
    "Add-ons",
    "Themes"
};
static const int NUM_TOPMENU_ITEMS = 6;

// Grid Variables
static uint32_t grid_selected_x = 0;
static uint32_t grid_selected_y = 0;
static uint32_t grid_scroll_y = 0;
static const int GRID_COLS = 4;
static const int GRID_COVER_W = 160;
static const int GRID_COVER_H = 160;
static const int GRID_PADDING = 20;

void store_ui_init(void) {
    current_state = STORE_STATE_GRID;
    topmenu_selected = 1;
    grid_selected_x = 0;
    grid_selected_y = 0;
    grid_scroll_y = 0;
}

void store_ui_start(void) {
    current_state = STORE_STATE_GRID;
}

static void draw_background(void) {
    // 2008 Store: Deep ocean blue background
    pkgi_draw_fill_rect(0, 0, VITA_WIDTH, VITA_HEIGHT, 0xFF441100); // 0xAABBGGRR - Deep blue: R=00, G=11, B=44
}

static void draw_top_bar(void) {
    // Top bar area
    pkgi_draw_fill_rect(0, 0, VITA_WIDTH, TOPBAR_HEIGHT, 0x55000000); // Translucent black overlay
    
    // Top Menu Items (Horizontal)
    int start_x = 50;
    for (int i = 0; i < NUM_TOPMENU_ITEMS; i++) {
        if (i == topmenu_selected && current_state == STORE_STATE_CATEGORIES) {
            // Highlighted top category
            pkgi_draw_fill_rect(start_x - 10, 35, pkgi_text_width(topmenu_items[i]) + 20, 40, 0xFFDDDDDD);
            pkgi_draw_text(start_x, 45, 0xFF000000, topmenu_items[i]);
        } else {
            pkgi_draw_text(start_x, 45, (i == topmenu_selected) ? 0xFFFFFFFF : 0xFFAAAAAA, topmenu_items[i]);
        }
        start_x += pkgi_text_width(topmenu_items[i]) + 40;
    }
}

static void draw_grid_view(pkgi_input* input) {
    uint32_t db_count = pkgi_db_count();
    
    // Input Handling
    if (input) {
        if (current_state == STORE_STATE_CATEGORIES) {
            if (input->pressed & PKGI_BUTTON_RIGHT) {
                if (topmenu_selected < NUM_TOPMENU_ITEMS - 1) topmenu_selected++;
            }
            if (input->pressed & PKGI_BUTTON_LEFT) {
                if (topmenu_selected > 0) topmenu_selected--;
            }
            if (input->pressed & PKGI_BUTTON_DOWN) {
                current_state = STORE_STATE_GRID; // Move down into the grid
            }
        } else if (current_state == STORE_STATE_GRID) {
            if (input->pressed & PKGI_BUTTON_LEFT) {
                if (grid_selected_x > 0) grid_selected_x--;
            }
            if (input->pressed & PKGI_BUTTON_RIGHT) {
                if (grid_selected_x < GRID_COLS - 1) grid_selected_x++;
            }
            if (input->pressed & PKGI_BUTTON_DOWN) {
                grid_selected_y++;
            }
            if (input->pressed & PKGI_BUTTON_UP) {
                if (grid_selected_y > 0) {
                    grid_selected_y--;
                } else {
                    current_state = STORE_STATE_CATEGORIES; // Move up into top menu
                }
            }
        }
    }

    // Scroll logic for grid
    if (grid_selected_y < grid_scroll_y) grid_scroll_y = grid_selected_y;
    if (grid_selected_y > grid_scroll_y + 2) grid_scroll_y = grid_selected_y - 2;

    int start_x = (VITA_WIDTH - (GRID_COLS * (GRID_COVER_W + GRID_PADDING))) / 2; // Centered
    int start_y = TOPBAR_HEIGHT + 40;
    
    // Draw the actual items
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < GRID_COLS; x++) {
            uint32_t item_index = (grid_scroll_y + y) * GRID_COLS + x;
            if (item_index >= db_count) continue;
            
            DbItem* item = pkgi_db_get(item_index);
            
            int draw_x = start_x + (x * (GRID_COVER_W + GRID_PADDING));
            int draw_y = start_y + (y * (GRID_COVER_H + GRID_PADDING));
            
            // Highlight rendering (2008 white/cyan glow)
            if (current_state == STORE_STATE_GRID && x == grid_selected_x && (grid_scroll_y + y) == grid_selected_y) {
                pkgi_draw_fill_rect_z(draw_x - 6, draw_y - 6, PKGI_FONT_Z, GRID_COVER_W + 12, GRID_COVER_H + 12, 0xFFFFFFFF);
                pkgi_draw_fill_rect_z(draw_x - 3, draw_y - 3, PKGI_FONT_Z, GRID_COVER_W + 6, GRID_COVER_H + 6, 0xFFFFDD00); // Cyan inner
            }
            
            // Item Background Box (Dark blue placeholder)
            pkgi_draw_fill_rect_z(draw_x, draw_y, PKGI_FONT_Z, GRID_COVER_W, GRID_COVER_H, 0xFF662211);
            
            // Truncated Text
            pkgi_clip_set(draw_x, draw_y, GRID_COVER_W, GRID_COVER_H);
            pkgi_draw_text_ttf(draw_x + 10, draw_y + GRID_COVER_H - 40, PKGI_FONT_Z, 0xFFFFFFFF, item->name);
            pkgi_clip_remove();
            
            // Details transition
            if (input && current_state == STORE_STATE_GRID && (input->pressed & pkgi_ok_button()) && x == grid_selected_x && (grid_scroll_y + y) == grid_selected_y) {
                input->pressed &= ~pkgi_ok_button();
                current_state = STORE_STATE_DETAILS;
            }
        }
    }
}

static void draw_details_view(pkgi_input* input) {
    uint32_t item_index = grid_selected_y * GRID_COLS + grid_selected_x;
    DbItem* item = pkgi_db_get(item_index);
    
    // Large cover image placeholder
    pkgi_draw_fill_rect(50, TOPBAR_HEIGHT + 50, 300, 300, 0xFF662211);
    
    pkgi_draw_text(400, TOPBAR_HEIGHT + 50, 0xFFFFFFFF, item->name);
    pkgi_draw_text(400, TOPBAR_HEIGHT + 100, 0xFFAAAAAA, "Content Type: PlayStation(R)3 Format Software");
    
    // Fake 2008 "Download" Button
    pkgi_draw_fill_rect(400, TOPBAR_HEIGHT + 200, 200, 50, 0xFF444444);
    pkgi_draw_text(440, TOPBAR_HEIGHT + 215, 0xFFFFFFFF, "[X] Download");
    
    pkgi_draw_text(50, VITA_HEIGHT - 50, 0xFFFFFFFF, "Press [O] to Go Back");
    
    if (input) {
        if (input->pressed & pkgi_cancel_button()) {
            input->pressed &= ~pkgi_cancel_button();
            current_state = STORE_STATE_GRID;
        } else if (input->pressed & pkgi_ok_button()) {
            input->pressed &= ~pkgi_ok_button();
            pkgi_download(item, 1);
            pkgi_dialog_start_progress("Downloading...", "Starting background DL...", 0);
        }
    }
}

void store_ui_do_main(pkgi_input* input) {
    draw_background();
    draw_top_bar();
    
    if (current_state == STORE_STATE_DETAILS) {
        draw_details_view(input);
    } else {
        draw_grid_view(input);
    }
}
