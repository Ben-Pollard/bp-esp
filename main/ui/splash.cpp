#include "splash.h"

void show_splash(lv_obj_t *scr, blockhaus_ident_done_cb_t done, void *user_data)
{
    blockhaus_ident_show(scr, "bp-esp", done, user_data);
}