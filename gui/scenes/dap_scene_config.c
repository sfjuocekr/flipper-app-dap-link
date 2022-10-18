#include "../dap_gui_i.h"

void dap_scene_config_on_enter(void* context) {
    DapGuiApp* app = context;
    UNUSED(app);
}

bool dap_scene_config_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void dap_scene_config_on_exit(void* context) {
    UNUSED(context);
}