#pragma once
#include <furi_hal_usb.h>

extern FuriHalUsbInterface dap_v2_usb_hid;

// receive callback type
typedef void (*DapRxCallback)(void* context);

typedef void (*DapStateCallback)(bool state, void* context);

bool dap_v1_usb_tx(uint8_t* buffer, uint8_t size);

size_t dap_v1_usb_rx(uint8_t* buffer, size_t size);

void dap_v1_usb_set_rx_callback(DapRxCallback callback);

bool dap_v2_usb_tx(uint8_t* buffer, uint8_t size);

size_t dap_v2_usb_rx(uint8_t* buffer, size_t size);

void dap_v2_usb_set_rx_callback(DapRxCallback callback);

void dap_common_usb_set_context(void* context);

void dap_common_usb_set_state_callback(DapStateCallback callback);