#pragma once
#include <furi_hal_usb.h>

extern FuriHalUsbInterface dap_v2_usb_hid;

// receive callback type
typedef void (*DapRxCallback)(void* context);

typedef void (*DapStateCallback)(bool state, void* context);

/************************************ V1 ***************************************/

int32_t dap_v1_usb_tx(uint8_t* buffer, uint8_t size);

size_t dap_v1_usb_rx(uint8_t* buffer, size_t size);

void dap_v1_usb_set_rx_callback(DapRxCallback callback);

/************************************ V2 ***************************************/

int32_t dap_v2_usb_tx(uint8_t* buffer, uint8_t size);

size_t dap_v2_usb_rx(uint8_t* buffer, size_t size);

void dap_v2_usb_set_rx_callback(DapRxCallback callback);

/************************************ CDC **************************************/

int32_t dap_cdc_usb_tx(uint8_t* buffer, uint8_t size);

size_t dap_cdc_usb_rx(uint8_t* buffer, size_t size);

void dap_cdc_usb_set_rx_callback(DapRxCallback callback);

/*********************************** Common ************************************/

void dap_common_usb_set_context(void* context);

void dap_common_usb_set_state_callback(DapStateCallback callback);

void dap_common_usb_alloc_name(const char* name);

void dap_common_usb_free_name();