#pragma once
#include <lvgl.h>
#include "blockhaus_ident.h"

class Supervisor;

void show_splash(lv_obj_t *scr, blockhaus_ident_done_cb_t done, void *user_data);
void start_main_ui(Supervisor *sup, int delay_ms);