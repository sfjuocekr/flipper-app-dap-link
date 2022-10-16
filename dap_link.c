#include <furi.h>
#include <furi_hal_version.h>
#include <furi_hal_gpio.h>
#include <furi_hal_uart.h>
#include <furi_hal_resources.h>
#include <m-string.h>
#include <dap.h>
#include "usb/dap_v2_usb.h"
#include "dap_config.h"

/***************************************************************************/
/****************************** DAP COMMON *********************************/
/***************************************************************************/

#define DAP_PROCESS_THREAD_TIMEOUT 500

typedef enum {
    DapThreadEventStop = (1 << 0),
} DapThreadEvent;

bool dap_thread_check_for_stop() {
    uint32_t flags = furi_thread_flags_get();
    return (flags & DapThreadEventStop);
}

GpioPin flipper_dap_swclk_pin;
GpioPin flipper_dap_swdio_pin;
GpioPin flipper_dap_reset_pin;
GpioPin flipper_dap_tdo_pin;
GpioPin flipper_dap_tdi_pin;

/***************************************************************************/
/****************************** DAP PROCESS ********************************/
/***************************************************************************/

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

#define USB_SERIAL_NUMBER_LEN 16
char usb_serial_number[USB_SERIAL_NUMBER_LEN] = {0};

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

static void dap_app_process_v1() {
    DapPacket tx_packet;
    DapPacket rx_packet;
    memset(&tx_packet, 0, sizeof(DapPacket));
    rx_packet.size = dap_v1_usb_rx(rx_packet.data, DAP_CONFIG_PACKET_SIZE);
    dap_process_request(rx_packet.data, rx_packet.size, tx_packet.data, DAP_CONFIG_PACKET_SIZE);
    dap_v1_usb_tx(tx_packet.data, DAP_CONFIG_PACKET_SIZE);
}

static void dap_app_process_v2() {
    DapPacket tx_packet;
    DapPacket rx_packet;
    memset(&tx_packet, 0, sizeof(DapPacket));
    rx_packet.size = dap_v2_usb_rx(rx_packet.data, DAP_CONFIG_PACKET_SIZE);
    size_t len = dap_process_request(
        rx_packet.data, rx_packet.size, tx_packet.data, DAP_CONFIG_PACKET_SIZE);
    dap_v2_usb_tx(tx_packet.data, len);
}

static int32_t dap_process(void* p) {
    UNUSED(p);

    // allocate resources
    FuriMessageQueue* queue = furi_message_queue_alloc(64, sizeof(DapMessage));
    FuriHalUsbInterface* usb_config_prev;

    // init dap
    dap_init();

    // get name
    const char* name = furi_hal_version_get_name_ptr();
    if(!name) {
        name = "Flipper";
    }
    snprintf(usb_serial_number, USB_SERIAL_NUMBER_LEN, "DAP_%s", name);

    // init usb
    usb_config_prev = furi_hal_usb_get_config();
    dap_common_usb_alloc_name(usb_serial_number);
    dap_common_usb_set_context(queue);
    dap_v1_usb_set_rx_callback(dap_app_rx1_callback);
    dap_v2_usb_set_rx_callback(dap_app_rx2_callback);
    furi_hal_usb_set_config(&dap_v2_usb_hid, NULL);

    // work
    DapMessage message;
    while(1) {
        if(furi_message_queue_get(queue, &message, DAP_PROCESS_THREAD_TIMEOUT) == FuriStatusOk) {
            switch(message.event) {
            case DapAppRxV1:
                dap_app_process_v1();
                break;
            case DapAppRxV2:
                dap_app_process_v2();
                break;
            }
        }

        if(dap_thread_check_for_stop()) {
            break;
        }
    }

    // deinit usb
    furi_hal_usb_set_config(usb_config_prev, NULL);
    dap_common_usb_free_name();

    // free resources
    furi_message_queue_free(queue);

    return 0;
}

/***************************************************************************/
/****************************** CDC PROCESS ********************************/
/***************************************************************************/

typedef enum {
    CDCThreadEventStop = DapThreadEventStop,
    CDCThreadEventUARTRx = (1 << 1),
    CDCThreadEventCDCRx = (1 << 2),
    CDCThreadEventCDCConfig = (1 << 3),
    CDCThreadEventAll = CDCThreadEventStop | CDCThreadEventUARTRx | CDCThreadEventCDCRx |
                        CDCThreadEventCDCConfig,
} CDCThreadEvent;

typedef struct {
    FuriStreamBuffer* rx_stream;
    FuriThreadId thread_id;
    FuriHalUartId uart_id;
    struct usb_cdc_line_coding line_coding;
} CDCProcess;

static void cdc_uart_irq_cb(UartIrqEvent ev, uint8_t data, void* ctx) {
    CDCProcess* app = ctx;

    if(ev == UartIrqEventRXNE) {
        furi_stream_buffer_send(app->rx_stream, &data, 1, 0);
        furi_thread_flags_set(app->thread_id, CDCThreadEventUARTRx);
    }
}

static void cdc_usb_rx_callback(void* context) {
    CDCProcess* app = context;
    furi_thread_flags_set(app->thread_id, CDCThreadEventCDCRx);
}

static void cdc_usb_control_line_callback(uint8_t state, void* context) {
    UNUSED(context);
    UNUSED(state);
}

static void cdc_usb_config_callback(struct usb_cdc_line_coding* config, void* context) {
    CDCProcess* app = context;
    app->line_coding = *config;
    furi_thread_flags_set(app->thread_id, CDCThreadEventCDCConfig);
}

static int32_t cdc_process(void* p) {
    UNUSED(p);
    CDCProcess* app = malloc(sizeof(CDCProcess));
    app->thread_id = furi_thread_get_id(furi_thread_get_current());
    app->rx_stream = furi_stream_buffer_alloc(512, 1);
    app->uart_id = FuriHalUartIdLPUART1;

    const uint8_t rx_buffer_size = 64;
    uint8_t* rx_buffer = malloc(rx_buffer_size);

    furi_hal_uart_init(app->uart_id, 115200);
    furi_hal_uart_set_irq_cb(app->uart_id, cdc_uart_irq_cb, app);

    dap_cdc_usb_set_context(app);
    dap_cdc_usb_set_rx_callback(cdc_usb_rx_callback);
    dap_cdc_usb_set_control_line_callback(cdc_usb_control_line_callback);
    dap_cdc_usb_set_config_callback(cdc_usb_config_callback);

    while(1) {
        uint32_t events =
            furi_thread_flags_wait(CDCThreadEventAll, FuriFlagWaitAny, FuriWaitForever);
        if(!(events & FuriFlagError)) {
            if(events & CDCThreadEventCDCConfig) {
                if(app->line_coding.dwDTERate > 0) {
                    furi_hal_uart_set_br(app->uart_id, app->line_coding.dwDTERate);
                }
            }

            if(events & CDCThreadEventUARTRx) {
                size_t len =
                    furi_stream_buffer_receive(app->rx_stream, rx_buffer, rx_buffer_size, 0);

                if(len > 0) {
                    dap_cdc_usb_tx(rx_buffer, len);
                }
            }

            if(events & CDCThreadEventCDCRx) {
                size_t len = dap_cdc_usb_rx(rx_buffer, rx_buffer_size);
                if(len > 0) {
                    furi_hal_uart_tx(app->uart_id, rx_buffer, len);
                }
            }

            if(events & CDCThreadEventStop) {
                break;
            }
        }
    }

    furi_hal_uart_set_irq_cb(app->uart_id, NULL, NULL);
    furi_hal_uart_deinit(app->uart_id);
    free(rx_buffer);
    furi_stream_buffer_free(app->rx_stream);
    free(app);

    return 0;
}

/***************************************************************************/
/******************************* MAIN APP **********************************/
/***************************************************************************/

typedef struct {
    FuriThread* dap_thread;
    FuriThread* cdc_thread;
    FuriThread* gui_thread;
} DapApp;

static FuriThread* furi_thread_alloc_ex(
    const char* name,
    uint32_t stack_size,
    FuriThreadCallback callback,
    void* context) {
    FuriThread* thread = furi_thread_alloc();
    furi_thread_set_name(thread, name);
    furi_thread_set_stack_size(thread, stack_size);
    furi_thread_set_callback(thread, callback);
    furi_thread_set_context(thread, context);
    return thread;
}

static DapApp* dap_app_alloc() {
    DapApp* dap_app = malloc(sizeof(DapApp));
    dap_app->dap_thread = furi_thread_alloc_ex("DAP", 1024, dap_process, dap_app);
    dap_app->cdc_thread = furi_thread_alloc_ex("CDC", 1024, cdc_process, dap_app);
    return dap_app;
}

static void dap_app_free(DapApp* dap_app) {
    furi_assert(dap_app);
    furi_thread_free(dap_app->dap_thread);
    furi_thread_free(dap_app->cdc_thread);
    free(dap_app);
}

int32_t dap_link_app(void* p) {
    UNUSED(p);

    // setup default pins
    flipper_dap_swclk_pin = gpio_ext_pa7;
    flipper_dap_swdio_pin = gpio_ext_pa6;
    flipper_dap_reset_pin = gpio_ext_pa4;
    flipper_dap_tdo_pin = gpio_ext_pb3;
    flipper_dap_tdi_pin = gpio_ext_pb2;

    // alloc app
    DapApp* app = dap_app_alloc();
    furi_thread_start(app->dap_thread);
    furi_thread_start(app->cdc_thread);

    // wait for threads to stop
    furi_thread_join(app->dap_thread);
    furi_thread_join(app->cdc_thread);

    // free app
    dap_app_free(app);

    // setup gpio pins to default state
    furi_hal_gpio_init(&flipper_dap_swclk_pin, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(&flipper_dap_swdio_pin, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(&flipper_dap_reset_pin, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(&flipper_dap_tdo_pin, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(&flipper_dap_tdi_pin, GpioModeAnalog, GpioPullNo, GpioSpeedLow);

    return 0;
}