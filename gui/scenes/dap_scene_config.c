#include "../dap_gui_i.h"

static const char* swd_pins[] = {"2,3", "10,12"};
static const char* uart_pins[] = {"13,14", "15,16"};
static const char* uart_swap[] = {"No", "Yes"};

static void swd_pins_cb(VariableItem* item) {
    DapGuiApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);

    variable_item_set_current_value_text(item, swd_pins[index]);

    app->config->swd_pins = index;
}

static void uart_pins_cb(VariableItem* item) {
    DapGuiApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);

    variable_item_set_current_value_text(item, uart_pins[index]);

    app->config->uart_pins = index;
}

static void uart_swap_cb(VariableItem* item) {
    DapGuiApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);

    variable_item_set_current_value_text(item, uart_swap[index]);

    app->config->uart_swap = index;
}

void dap_scene_config_on_enter(void* context) {
    DapGuiApp* app = context;
    VariableItemList* var_item_list = app->var_item_list;
    VariableItem* item;

    item = variable_item_list_add(var_item_list, "SWC/D Pins", 2, swd_pins_cb, app);
    variable_item_set_current_value_index(item, app->config->swd_pins);
    variable_item_set_current_value_text(item, swd_pins[app->config->swd_pins]);

    item = variable_item_list_add(var_item_list, "UART Pins", 2, uart_pins_cb, app);
    variable_item_set_current_value_index(item, app->config->uart_pins);
    variable_item_set_current_value_text(item, uart_pins[app->config->uart_pins]);

    item = variable_item_list_add(var_item_list, "Swap TX/RX", 2, uart_swap_cb, app);
    variable_item_set_current_value_index(item, app->config->uart_swap);
    variable_item_set_current_value_text(item, uart_swap[app->config->uart_swap]);

    variable_item_list_set_selected_item(
        var_item_list, scene_manager_get_scene_state(app->scene_manager, DapSceneConfig));
    view_dispatcher_switch_to_view(app->view_dispatcher, DapGuiAppViewVarItemList);
}

bool dap_scene_config_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void dap_scene_config_on_exit(void* context) {
    DapGuiApp* app = context;
    scene_manager_set_scene_state(
        app->scene_manager,
        DapSceneConfig,
        variable_item_list_get_selected_item_index(app->var_item_list));
    variable_item_list_reset(app->var_item_list);
}