#include "store_ui.h"
#include "pkgi_menu.h"
#include "pkgi_config.h"
#include "pkgi_dialog.h"
#include "pkgi_download.h"
#include "pkgi_utils.h"
#include "pkgi_style.h"

static StoreState current_state = STORE_STATE_GRID;

// View state variables
static uint32_t grid_selected_x = 0;
static uint32_t grid_selected_y = 0;
static uint32_t grid_scroll_y = 0;
static const int GRID_COLS = 4;
static const int GRID_COVER_W = 150;
static const int GRID_COVER_H = 150;
static const int GRID_PADDING = 20;

void store_ui_init(void) {
    current_state = STORE_STATE_GRID;
    grid_selected_x = 0;
    grid_selected_y = 0;
    grid_scroll_y = 0;
}

void store_ui_start(void) {
    // Reset any UI specific elements when returning to the store
    current_state = STORE_STATE_GRID;
}

static void draw_top_bar(void) {
    pkgi_draw_fill_rect(0, 0, VITA_WIDTH, 60, PKGI_COLOR_HLINE);
    pkgi_draw_text(20, 20, PKGI_COLOR_TEXT_HEAD, "PlayStation Store (NPS)");
}

static void draw_grid_view(pkgi_input* input) {
    uint32_t db_count = pkgi_db_count();
    
    // Handle input
    if (input) {
        if (input->pressed & PKGI_BUTTON_RIGHT) {
            if (grid_selected_x < GRID_COLS - 1) grid_selected_x++;
        }
        if (input->pressed & PKGI_BUTTON_LEFT) {
            if (grid_selected_x > 0) grid_selected_x--;
        }
        if (input->pressed & PKGI_BUTTON_DOWN) {
            grid_selected_y++;
        }
        if (input->pressed & PKGI_BUTTON_UP) {
            if (grid_selected_y > 0) grid_selected_y--;
        }
    }
    
    // Ensure scroll follows selection
    if (grid_selected_y < grid_scroll_y) grid_scroll_y = grid_selected_y;
    if (grid_selected_y > grid_scroll_y + 2) grid_scroll_y = grid_selected_y - 2;

    int start_x = 200; // Offset for left sidebar
    int start_y = 80;
    
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < GRID_COLS; x++) {
            uint32_t item_index = (grid_scroll_y + y) * GRID_COLS + x;
            if (item_index >= db_count) continue;
            
            DbItem* item = pkgi_db_get(item_index);
            
            int draw_x = start_x + (x * (GRID_COVER_W + GRID_PADDING));
            int draw_y = start_y + (y * (GRID_COVER_H + GRID_PADDING));
            
            // Draw placeholder cover box
            uint32_t color = (x == grid_selected_x && (grid_scroll_y + y) == grid_selected_y) ? PKGI_COLOR_SELECTED_BACKGROUND : PKGI_COLOR_BACKGROUND;
            pkgi_draw_fill_rect_z(draw_x, draw_y, PKGI_FONT_Z, GRID_COVER_W, GRID_COVER_H, color);
            
            // Draw text over placeholder
            pkgi_clip_set(draw_x, draw_y, GRID_COVER_W, GRID_COVER_H);
            pkgi_draw_text_ttf(draw_x + 5, draw_y + 5, PKGI_FONT_Z, PKGI_COLOR_TEXT, item->name);
            pkgi_clip_remove();
            
            // Interaction
            if (input && (input->pressed & pkgi_ok_button()) && (x == grid_selected_x && (grid_scroll_y + y) == grid_selected_y)) {
                input->pressed &= ~pkgi_ok_button();
                current_state = STORE_STATE_DETAILS;
                // Save selection info for details view...
            }
        }
    }
}

static void draw_details_view(pkgi_input* input) {
    uint32_t item_index = grid_selected_y * GRID_COLS + grid_selected_x;
    DbItem* item = pkgi_db_get(item_index);
    
    pkgi_draw_text(200, 100, PKGI_COLOR_TEXT_HEAD, item->name);
    pkgi_draw_text(200, 150, PKGI_COLOR_TEXT, "Press X to Download");
    pkgi_draw_text(200, 200, PKGI_COLOR_TEXT, "Press O to Go Back");
    
    if (input) {
        if (input->pressed & pkgi_cancel_button()) {
            input->pressed &= ~pkgi_cancel_button();
            current_state = STORE_STATE_GRID;
        } else if (input->pressed & pkgi_ok_button()) {
            input->pressed &= ~pkgi_ok_button();
            // Start download
            pkgi_download(item, 1);
            pkgi_dialog_start_progress("Downloading...", "Starting background DL...", 0);
        }
    }
}

void store_ui_do_main(pkgi_input* input) {
    draw_top_bar();
    
    switch (current_state) {
        case STORE_STATE_GRID:
            draw_grid_view(input);
            break;
        case STORE_STATE_DETAILS:
            draw_details_view(input);
            break;
        default:
            break;
    }
}
