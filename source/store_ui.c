#include "store_ui.h"
#include "pkgi_menu.h"
#include "pkgi_config.h"
#include "pkgi_dialog.h"
#include "pkgi_download.h"
#include "pkgi_utils.h"
#include "pkgi_style.h"

static StoreState current_state = STORE_STATE_CATEGORIES;

// 2006 Store Layout Constants
static const int SIDEBAR_WIDTH = 250;
static const int TOPBAR_HEIGHT = 70;

// Sidebar Variables
static int sidebar_selected = 0;
static const char* sidebar_items[] = {
    "Latest",
    "Games",
    "Game Add-ons",
    "Themes & Avatars",
    "Search"
};
static const int NUM_SIDEBAR_ITEMS = 5;

// Grid Variables
static uint32_t grid_selected_x = 0;
static uint32_t grid_selected_y = 0;
static uint32_t grid_scroll_y = 0;
static const int GRID_COLS = 3;
static const int GRID_COVER_W = 160;
static const int GRID_COVER_H = 160;
static const int GRID_PADDING = 15;

void store_ui_init(void) {
    current_state = STORE_STATE_CATEGORIES;
    sidebar_selected = 0;
    grid_selected_x = 0;
    grid_selected_y = 0;
    grid_scroll_y = 0;
}

void store_ui_start(void) {
    current_state = STORE_STATE_CATEGORIES;
}

static void draw_top_bar(void) {
    // 2006 Store Top Bar: Very dark blue/black header
    pkgi_draw_fill_rect(0, 0, VITA_WIDTH, TOPBAR_HEIGHT, 0x00111111); // Dark charcoal
    pkgi_draw_text(20, 25, 0x00FFFFFF, "PLAYSTATION(R)Store");
    pkgi_draw_text(VITA_WIDTH - 200, 25, 0x00AAAAAA, "View Cart | Settings");
}

static void draw_sidebar(void) {
    // 2006 Store Left Sidebar: Dark translucent background
    pkgi_draw_fill_rect(0, TOPBAR_HEIGHT, SIDEBAR_WIDTH, VITA_HEIGHT - TOPBAR_HEIGHT, 0xAA000000);

    for (int i = 0; i < NUM_SIDEBAR_ITEMS; i++) {
        int y = TOPBAR_HEIGHT + 30 + (i * 50);
        
        if (i == sidebar_selected && current_state == STORE_STATE_CATEGORIES) {
            // Selected item: Light blue background highlight (Classic 2006 style)
            pkgi_draw_fill_rect(10, y - 10, SIDEBAR_WIDTH - 20, 40, 0x0088CCFF);
            pkgi_draw_text(20, y, 0x00000000, sidebar_items[i]); // Black text on blue
        } else {
            // Unselected item
            pkgi_draw_text(20, y, 0x00FFFFFF, sidebar_items[i]);
        }
    }
}

static void draw_grid_view(pkgi_input* input) {
    uint32_t db_count = pkgi_db_count();
    
    // Input Handling
    if (input) {
        if (current_state == STORE_STATE_CATEGORIES) {
            if (input->pressed & PKGI_BUTTON_DOWN) {
                if (sidebar_selected < NUM_SIDEBAR_ITEMS - 1) sidebar_selected++;
            }
            if (input->pressed & PKGI_BUTTON_UP) {
                if (sidebar_selected > 0) sidebar_selected--;
            }
            if (input->pressed & PKGI_BUTTON_RIGHT) {
                current_state = STORE_STATE_GRID; // Move focus to grid
            }
        } else if (current_state == STORE_STATE_GRID) {
            if (input->pressed & PKGI_BUTTON_LEFT) {
                if (grid_selected_x > 0) {
                    grid_selected_x--;
                } else {
                    current_state = STORE_STATE_CATEGORIES; // Move focus back to sidebar
                }
            }
            if (input->pressed & PKGI_BUTTON_RIGHT) {
                if (grid_selected_x < GRID_COLS - 1) grid_selected_x++;
            }
            if (input->pressed & PKGI_BUTTON_DOWN) {
                grid_selected_y++;
            }
            if (input->pressed & PKGI_BUTTON_UP) {
                if (grid_selected_y > 0) grid_selected_y--;
            }
        }
    }

    // Scroll logic for grid
    if (grid_selected_y < grid_scroll_y) grid_scroll_y = grid_selected_y;
    if (grid_selected_y > grid_scroll_y + 2) grid_scroll_y = grid_selected_y - 2;

    int start_x = SIDEBAR_WIDTH + 30; // Grid starts right of the sidebar
    int start_y = TOPBAR_HEIGHT + 30;
    
    // Draw background for content area (Classic blue gradient feel)
    pkgi_draw_fill_rect(SIDEBAR_WIDTH, TOPBAR_HEIGHT, VITA_WIDTH - SIDEBAR_WIDTH, VITA_HEIGHT - TOPBAR_HEIGHT, 0x00082444);

    // Draw the actual items
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < GRID_COLS; x++) {
            uint32_t item_index = (grid_scroll_y + y) * GRID_COLS + x;
            if (item_index >= db_count) continue;
            
            DbItem* item = pkgi_db_get(item_index);
            
            int draw_x = start_x + (x * (GRID_COVER_W + GRID_PADDING));
            int draw_y = start_y + (y * (GRID_COVER_H + GRID_PADDING));
            
            // Highlight rendering
            if (current_state == STORE_STATE_GRID && x == grid_selected_x && (grid_scroll_y + y) == grid_selected_y) {
                // 2006 Orange/Yellow highlight glow border
                pkgi_draw_fill_rect_z(draw_x - 4, draw_y - 4, PKGI_FONT_Z, GRID_COVER_W + 8, GRID_COVER_H + 8, 0x00FFAA00);
            }
            
            // Item Background Box (Dark blue/black)
            pkgi_draw_fill_rect_z(draw_x, draw_y, PKGI_FONT_Z, GRID_COVER_W, GRID_COVER_H, 0x00112233);
            
            // Truncated Text
            pkgi_clip_set(draw_x, draw_y, GRID_COVER_W, GRID_COVER_H);
            pkgi_draw_text_ttf(draw_x + 5, draw_y + GRID_COVER_H - 30, PKGI_FONT_Z, PKGI_COLOR_TEXT, item->name);
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
    
    // Details background
    pkgi_draw_fill_rect(0, TOPBAR_HEIGHT, VITA_WIDTH, VITA_HEIGHT - TOPBAR_HEIGHT, 0x00082444);
    
    pkgi_draw_text(100, 150, 0x00FFFFFF, item->name);
    pkgi_draw_text(100, 200, 0x00AAAAAA, "Content Type: PKG");
    
    // Fake 2006 "Download" Button (Rounded pill look via rects)
    pkgi_draw_fill_rect(100, 300, 200, 50, 0x0088CCFF);
    pkgi_draw_text(140, 315, 0x00000000, "[X] Download");
    
    pkgi_draw_text(100, 400, 0x00FFFFFF, "Press [O] to Go Back");
    
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
    draw_top_bar();
    
    if (current_state == STORE_STATE_DETAILS) {
        draw_details_view(input);
    } else {
        draw_sidebar();
        draw_grid_view(input);
    }
}
