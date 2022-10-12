#include <furi.h>
#include <furi_hal_resources.h>
#include <m-string.h>
#include "dap.h"
#include "dap_usb.h"
#include "dap_v2_usb.h"
#include "dap_config.h"

typedef enum {
    DapVersion1,
    DapVersion2,
} DapVersion;

#define DAP_DEFAULT_VERSION DapVersion2

typedef struct {
    bool (*tx)(uint8_t* buffer, uint8_t size);
    void (*set_context)(void* context);
    void (*set_rx_callback)(DapRxCallback callback);
    void (*set_state_callback)(DapStateCallback callback);
    FuriHalUsbInterface* interface;
} DapLinkIf;

static DapLinkIf dap_interface[2] = {
    {
        .tx = dap_usb_tx,
        .set_context = dap_usb_set_context,
        .set_rx_callback = dap_usb_set_rx_callback,
        .set_state_callback = dap_usb_set_state_callback,
        .interface = &dap_usb_hid,
    },
    {
        .tx = dap_v2_usb_tx,
        .set_context = dap_v2_usb_set_context,
        .set_rx_callback = dap_v2_usb_set_rx_callback,
        .set_state_callback = dap_v2_usb_set_state_callback,
        .interface = &dap_v2_usb_hid,
    },
};

typedef struct {
    FuriHalUsbInterface* usb_config_prev;
    FuriMessageQueue* queue;
} DapApp;

typedef struct {
    uint8_t data[DAP_CONFIG_PACKET_SIZE];
    uint8_t size;
} DapPacket;

typedef enum {
    DapAppRx,
} DapAppEvent;

typedef struct {
    DapAppEvent event;
    union {
        DapPacket packet;
    };
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
    'F',
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

static void dap_app_rx_callback(uint8_t* buffer, uint8_t size, void* context) {
    furi_assert(context);
    FuriMessageQueue* queue = context;
    DapMessage message = {
        .event = DapAppRx,
        .packet =
            {
                .size = size,
            },
    };
    memcpy(message.packet.data, buffer, size);
    furi_check(furi_message_queue_put(queue, &message, 0) == FuriStatusOk);
}

static void dap_app_state_callback(bool state, void* context) {
    UNUSED(state);
    UNUSED(context);
}

static void dap_app_usb_start(DapApp* dap_app) {
    furi_assert(dap_app);
    dap_app->usb_config_prev = furi_hal_usb_get_config();

    dap_interface[DAP_DEFAULT_VERSION].set_context(dap_app->queue);
    dap_interface[DAP_DEFAULT_VERSION].set_rx_callback(dap_app_rx_callback);
    dap_interface[DAP_DEFAULT_VERSION].set_state_callback(dap_app_state_callback);

    furi_hal_usb_set_config(dap_interface[DAP_DEFAULT_VERSION].interface, NULL);
}

static void dap_app_usb_stop(DapApp* dap_app) {
    furi_assert(dap_app);
    furi_hal_usb_set_config(dap_app->usb_config_prev, NULL);
    dap_interface[DAP_DEFAULT_VERSION].set_rx_callback(NULL);
    dap_interface[DAP_DEFAULT_VERSION].set_state_callback(NULL);
    dap_interface[DAP_DEFAULT_VERSION].set_context(NULL);
}

static void dap_app_process(DapApp* dap_app, DapPacket* rx_packet) {
    UNUSED(dap_app);
    DapPacket tx_packet;
    memset(&tx_packet, 0, sizeof(DapPacket));
    size_t len = dap_process_request(
        rx_packet->data, rx_packet->size, tx_packet.data, DAP_CONFIG_PACKET_SIZE);
    dap_interface[DAP_DEFAULT_VERSION].tx(tx_packet.data, len);
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
            case DapAppRx:
                dap_app_process(app, &message.packet);
                break;
            }
        }
    }

    dap_app_usb_stop(app);
    dap_app_free(app);

    return 0;
}