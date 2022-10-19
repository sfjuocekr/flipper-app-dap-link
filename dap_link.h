#pragma once
#include <stdint.h>

typedef enum {
    DapModeDisconnected,
    DapModeSWD,
    DapModeJTAG,
} DapMode;

typedef struct {
    DapMode dap_mode;
    uint32_t dap_counter;
    uint32_t cdc_baudrate;
    uint32_t cdc_tx_counter;
    uint32_t cdc_rx_counter;
} DapState;

typedef struct DapApp DapApp;

void dap_app_get_state(DapApp* app, DapState* state);