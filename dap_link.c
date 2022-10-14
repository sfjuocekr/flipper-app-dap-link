#include <furi.h>
#include <furi_hal_resources.h>
#include <m-string.h>
#include "dap.h"
#include "dap_v2_usb.h"
#include "dap_config.h"

typedef struct {
    FuriHalUsbInterface* usb_config_prev;
    FuriMessageQueue* queue;
} DapApp;

typedef struct {
    uint8_t data[DAP_CONFIG_PACKET_SIZE];
    uint8_t size;
} DapPacket;

typedef enum {
    DapAppRxV1,
    DapAppRxV2,
} DapAppEvent;

typedef struct {
    DapAppEvent event;
} DapMessage;

char usb_serial_number[16] = {
    '0',
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    'A',
    'B',
    'C',
    'D',
    'E',
    0,
};

static DapApp* dap_app_alloc() {
    DapApp* dap_app = malloc(sizeof(DapApp));
    dap_app->usb_config_prev = NULL;
    dap_app->queue = furi_message_queue_alloc(16, sizeof(DapMessage));
    return dap_app;
}

static void dap_app_free(DapApp* dap_app) {
    furi_assert(dap_app);
    free(dap_app);
}

static void dap_app_rx1_callback(void* context) {
    furi_assert(context);
    FuriMessageQueue* queue = context;
    DapMessage message = {.event = DapAppRxV1};

    furi_check(furi_message_queue_put(queue, &message, 0) == FuriStatusOk);
}

static void dap_app_rx2_callback(void* context) {
    furi_assert(context);
    FuriMessageQueue* queue = context;
    DapMessage message = {.event = DapAppRxV2};
    furi_check(furi_message_queue_put(queue, &message, 0) == FuriStatusOk);
}

static void dap_app_state_callback(bool state, void* context) {
    UNUSED(state);
    UNUSED(context);
}

static void dap_app_usb_start(DapApp* dap_app) {
    furi_assert(dap_app);
    dap_app->usb_config_prev = furi_hal_usb_get_config();

    dap_common_usb_set_state_callback(dap_app_state_callback);
    dap_common_usb_set_context(dap_app->queue);
    dap_v1_usb_set_rx_callback(dap_app_rx1_callback);
    dap_v2_usb_set_rx_callback(dap_app_rx2_callback);

    furi_hal_usb_set_config(&dap_v2_usb_hid, NULL);
}

static void dap_app_usb_stop(DapApp* dap_app) {
    furi_assert(dap_app);
    furi_hal_usb_set_config(dap_app->usb_config_prev, NULL);
    dap_v2_usb_set_rx_callback(NULL);
    dap_common_usb_set_state_callback(NULL);
    dap_v1_usb_set_rx_callback(NULL);
    dap_v2_usb_set_rx_callback(NULL);
}

void dap_app_log_buffer(const char* prefix, const uint8_t* buffer, uint32_t size) {
    furi_assert(prefix);
    furi_assert(buffer);
    furi_assert(size);

    FuriString* str = furi_string_alloc();
    for(uint32_t i = 0; i < size; i++) {
        furi_string_cat_printf(str, "%02X ", buffer[i]);
    }
    FURI_LOG_I("DAP", "%s: %s", prefix, furi_string_get_cstr(str));
    furi_string_free(str);
}

static void dap_app_process_v1(DapApp* dap_app) {
    UNUSED(dap_app);
    DapPacket tx_packet;
    DapPacket rx_packet;
    memset(&tx_packet, 0, sizeof(DapPacket));
    rx_packet.size = dap_v1_usb_rx(rx_packet.data, DAP_CONFIG_PACKET_SIZE);
    dap_process_request(rx_packet.data, rx_packet.size, tx_packet.data, DAP_CONFIG_PACKET_SIZE);
    dap_v1_usb_tx(tx_packet.data, DAP_CONFIG_PACKET_SIZE);
}

static void dap_app_process_v2(DapApp* dap_app) {
    UNUSED(dap_app);
    DapPacket tx_packet;
    DapPacket rx_packet;
    memset(&tx_packet, 0, sizeof(DapPacket));
    rx_packet.size = dap_v2_usb_rx(rx_packet.data, DAP_CONFIG_PACKET_SIZE);
    size_t len = dap_process_request(
        rx_packet.data, rx_packet.size, tx_packet.data, DAP_CONFIG_PACKET_SIZE);
    dap_v2_usb_tx(tx_packet.data, len);
    // dap_app_log_buffer("RX", rx_packet->data, rx_packet->size);
    // dap_app_log_buffer("TX", tx_packet.data, len);
}

int32_t dap_link_app(void* p) {
    UNUSED(p);

    DapApp* app = dap_app_alloc();
    dap_init();
    dap_app_usb_start(app);

    DapMessage message;
    while(1) {
        if(furi_message_queue_get(app->queue, &message, FuriWaitForever) == FuriStatusOk) {
            switch(message.event) {
            case DapAppRxV1:
                dap_app_process_v1(app);
                break;
            case DapAppRxV2:
                dap_app_process_v2(app);
                break;
            }
        }
    }

    dap_app_usb_stop(app);
    dap_app_free(app);

    return 0;
}