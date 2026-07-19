#pragma once

#include "pkgi.h"
#include "pkgi_db.h"

// Stubs for removed pkgi_menu
#define MenuResult int
#define MenuResultRefresh 1
#define pkgi_menu_is_open() (0)
#define pkgi_menu_result() (0)
#define pkgi_menu_start(search, cfg) ((void)0)
#define pkgi_menu_get(cfg) ((void)0)

typedef enum {
    STORE_STATE_CATEGORIES,
    STORE_STATE_GRID,
    STORE_STATE_DETAILS
} StoreState;

void store_ui_init(void);
void store_ui_start(void);
void store_ui_do_main(pkgi_input* input);
