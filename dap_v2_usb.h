#pragma once
#include "dap_usb.h"
#include <furi_hal_usb.h>

extern FuriHalUsbInterface dap_v2_usb_hid;

bool dap_v2_usb_tx(uint8_t* buffer, uint8_t size);

void dap_v2_usb_set_context(void* context);

void dap_v2_usb_set_rx_callback(DapRxCallback callback);

void dap_v2_usb_set_state_callback(DapStateCallback callback);