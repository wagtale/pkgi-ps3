#pragma once

#include "pkgi.h"
#include "pkgi_db.h"

typedef enum {
    STORE_STATE_CATEGORIES,
    STORE_STATE_GRID,
    STORE_STATE_DETAILS
} StoreState;

void store_ui_init(void);
void store_ui_start(void);
void store_ui_do_main(pkgi_input* input);
