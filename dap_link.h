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

typedef enum {
    DapSwdPinsPA7PA6, // Pins 2, 3
    DapSwdPinsPA14PA13, // Pins 10, 12

    DapSwdPinsCount,
} DapSwdPins;

typedef enum {
    DapUartTypeUSART1, // Pins 13, 14
    DapUartTypeLPUART1, // Pins 15, 16

    DapUartTypeCount,
} DapUartType;

typedef enum {
    DapUartTXRXNormal,
    DapUartTXRXSwap,

    DapUartTXRXCount,
} DapUartTXRX;

typedef struct {
    DapSwdPins swd_pins;
    DapUartType uart_pins;
    DapUartTXRX uart_swap;
} DapConfig;

typedef struct DapApp DapApp;

void dap_app_get_state(DapApp* app, DapState* state);

const char* dap_app_get_serial(DapApp* app);