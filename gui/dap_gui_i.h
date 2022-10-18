#pragma once

#include "dap_gui.h"

#include "scenes/config/dap_scene.h"
#include "dap_gui_custom_event.h"

#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <gui/modules/submenu.h>
#include <notification/notification_messages.h>
#include <gui/modules/variable_item_list.h>

typedef struct {
    Gui* gui;
    NotificationApp* notifications;
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;

    VariableItemList* var_item_list;
} DapGuiApp;

typedef enum {
    DapGuiAppViewVarItemList,
} DapGuiAppView;
