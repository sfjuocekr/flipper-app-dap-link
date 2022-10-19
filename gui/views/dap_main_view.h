#pragma once
#include <gui/view.h>

typedef struct DapMainView DapMainView;

typedef void (*DapMainViewButtonCallback)(void* context);

typedef enum {
    DapMainVewModeDisconnected,
    DapMainVewModeSWD,
    DapMainVewModeJTAG,
} DapMainVewMode;

DapMainView* dap_main_view_alloc();

void dap_main_view_free(DapMainView* dap_main_view);

View* dap_main_view_get_view(DapMainView* dap_main_view);

void dap_main_view_set_left_callback(
    DapMainView* dap_main_view,
    DapMainViewButtonCallback callback,
    void* context);

void dap_main_view_set_mode(DapMainView* dap_main_view, DapMainVewMode mode);

void dap_main_view_set_dap(DapMainView* dap_main_view, bool active);

void dap_main_view_set_tx(DapMainView* dap_main_view, bool active);

void dap_main_view_set_rx(DapMainView* dap_main_view, bool active);

void dap_main_view_set_baudrate(DapMainView* dap_main_view, uint32_t baudrate);

void dap_main_view_update(DapMainView* dap_main_view);