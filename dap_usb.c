#include <furi.h>
#include "usb.h"
#include "usb_std.h"
#include "usb_hid.h"
#include "dap_usb.h"
#include <furi_hal_console.h>

// #define DAP_USB_LOG

#define HID_EP_IN 0x80
#define HID_EP_OUT 0x00

#define DAP_HID_EP_SEND 1
#define DAP_HID_EP_RECV 2

#define DAP_HID_EP_IN (HID_EP_IN | DAP_HID_EP_SEND)
#define DAP_HID_EP_OUT (HID_EP_OUT | DAP_HID_EP_RECV)

#define DAP_HID_EP_SIZE 64

#define DAP_HID_INTERVAL 2

#define DAP_HID_VID 0x6666
#define DAP_HID_PID 0x9930

#define DAP_USB_EP0_SIZE 8

#define EP_CFG_DECONFIGURE 0
#define EP_CFG_CONFIGURE 1

struct HidConfigDescriptor {
    struct usb_config_descriptor configuration;
    struct usb_interface_descriptor hid_interface;
    struct usb_hid_descriptor hid;
    struct usb_endpoint_descriptor hid_ep_in;
    struct usb_endpoint_descriptor hid_ep_out;
} __attribute__((packed));

static const struct usb_device_descriptor hid_device_desc = {
    .bLength = sizeof(struct usb_device_descriptor),
    .bDescriptorType = USB_DTYPE_DEVICE,
    .bcdUSB = VERSION_BCD(2, 0, 0),
    .bDeviceClass = USB_CLASS_PER_INTERFACE,
    .bDeviceSubClass = USB_SUBCLASS_NONE,
    .bDeviceProtocol = USB_PROTO_NONE,
    .bMaxPacketSize0 = DAP_USB_EP0_SIZE,
    .idVendor = DAP_HID_VID,
    .idProduct = DAP_HID_PID,
    .bcdDevice = VERSION_BCD(1, 0, 0),
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1,
};

static const uint8_t hid_report_desc[] = {
    0x05, 0x01, // Usage Page (Generic Desktop Ctrls)
    0x09, 0x00, // Usage (Undefined)
    0xa1, 0x01, // Collection (Application)
    0x15, 0x00, //   Logical Minimum (0)
    0x26, 0xff, 0x00, //   Logical Maximum (255)
    0x75, 0x08, //   Report Size (8)
    0x95, 0x40, //   Report Count (64)
    0x09, 0x00, //   Usage (Undefined)
    0x81, 0x82, //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x08, //   Report Size (8)
    0x95, 0x40, //   Report Count (64)
    0x09, 0x00, //   Usage (Undefined)
    0x91, 0x82, //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Volatile)
    0xc0, // End Collection
};

static const struct HidConfigDescriptor hid_cfg_desc = {
    .configuration =
        {
            .bLength = sizeof(struct usb_config_descriptor),
            .bDescriptorType = USB_DTYPE_CONFIGURATION,
            .wTotalLength = sizeof(struct HidConfigDescriptor),
            .bNumInterfaces = 1,
            .bConfigurationValue = 1,
            .iConfiguration = NO_DESCRIPTOR,
            .bmAttributes = USB_CFG_ATTR_RESERVED,
            .bMaxPower = USB_CFG_POWER_MA(500),
        },

    .hid_interface =
        {
            .bLength = sizeof(struct usb_interface_descriptor),
            .bDescriptorType = USB_DTYPE_INTERFACE,
            .bInterfaceNumber = 0,
            .bAlternateSetting = 0,
            .bNumEndpoints = 2,
            .bInterfaceClass = USB_CLASS_HID,
            .bInterfaceSubClass = USB_HID_SUBCLASS_NONBOOT,
            .bInterfaceProtocol = USB_HID_PROTO_NONBOOT,
            .iInterface = NO_DESCRIPTOR,
        },

    .hid =
        {
            .bLength = sizeof(struct usb_hid_descriptor),
            .bDescriptorType = USB_DTYPE_HID,
            .bcdHID = VERSION_BCD(1, 1, 1),
            .bCountryCode = USB_HID_COUNTRY_NONE,
            .bNumDescriptors = 1,
            .bDescriptorType0 = USB_DTYPE_HID_REPORT,
            .wDescriptorLength0 = sizeof(hid_report_desc),
        },

    .hid_ep_in =
        {
            .bLength = sizeof(struct usb_endpoint_descriptor),
            .bDescriptorType = USB_DTYPE_ENDPOINT,
            .bEndpointAddress = DAP_HID_EP_IN,
            .bmAttributes = USB_EPTYPE_INTERRUPT,
            .wMaxPacketSize = DAP_HID_EP_SIZE,
            .bInterval = DAP_HID_INTERVAL,
        },

    .hid_ep_out =
        {
            .bLength = sizeof(struct usb_endpoint_descriptor),
            .bDescriptorType = USB_DTYPE_ENDPOINT,
            .bEndpointAddress = DAP_HID_EP_OUT,
            .bmAttributes = USB_EPTYPE_INTERRUPT,
            .wMaxPacketSize = DAP_HID_EP_SIZE,
            .bInterval = DAP_HID_INTERVAL,
        },
};

typedef struct {
    FuriSemaphore* semaphore;
    bool connected;
    usbd_device* usb_dev;
    DapStateCallback state_callback;
    DapRxCallback rx_callback;
    void* context;
} DAPState;

DAPState dap_state = {
    .semaphore = NULL,
    .connected = false,
    .usb_dev = NULL,
    .state_callback = NULL,
    .rx_callback = NULL,
    .context = NULL,
};

#ifdef DAP_USB_LOG
void furi_console_log_printf(const char* format, ...) _ATTRIBUTE((__format__(__printf__, 1, 2)));

void furi_console_log_printf(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    furi_hal_console_puts(buffer);
    furi_hal_console_puts("\r\n");
    UNUSED(format);
}
#else
#define furi_console_log_printf(...)
#endif

bool dap_usb_tx(uint8_t* buffer, uint8_t size) {
    if((dap_state.semaphore == NULL) || (dap_state.connected == false)) return false;

    furi_check(furi_semaphore_acquire(dap_state.semaphore, FuriWaitForever) == FuriStatusOk);

    if(dap_state.connected) {
        int32_t len = usbd_ep_write(dap_state.usb_dev, DAP_HID_EP_IN, buffer, size);
        UNUSED(len);
        furi_console_log_printf("tx %ld", len);
    }

    return false;
}

void dap_usb_set_context(void* context) {
    dap_state.context = context;
}

void dap_usb_set_rx_callback(DapRxCallback callback) {
    dap_state.rx_callback = callback;
}

void dap_usb_set_state_callback(DapStateCallback callback) {
    dap_state.state_callback = callback;
}

static void hid_init(usbd_device* dev, FuriHalUsbInterface* intf, void* ctx);
static void hid_deinit(usbd_device* dev);
static void hid_on_wakeup(usbd_device* dev);
static void hid_on_suspend(usbd_device* dev);

static usbd_respond hid_ep_config(usbd_device* dev, uint8_t cfg);
static usbd_respond hid_control(usbd_device* dev, usbd_ctlreq* req, usbd_rqc_callback* callback);

static const struct usb_string_descriptor dev_manuf_descr =
    USB_STRING_DESC("Flipper Devices Inc.");
static const struct usb_string_descriptor dev_prod_descr = USB_STRING_DESC("CMSIS-DAP Adapter");
static const struct usb_string_descriptor dev_serial_descr = USB_STRING_DESC("01234567890ABCDEF");

FuriHalUsbInterface dap_usb_hid = {
    .init = hid_init,
    .deinit = hid_deinit,
    .wakeup = hid_on_wakeup,
    .suspend = hid_on_suspend,

    .dev_descr = (struct usb_device_descriptor*)&hid_device_desc,

    .str_manuf_descr = (void*)&dev_manuf_descr,
    .str_prod_descr = (void*)&dev_prod_descr,
    .str_serial_descr = (void*)&dev_serial_descr,

    .cfg_descr = (void*)&hid_cfg_desc,
};

static void hid_init(usbd_device* dev, FuriHalUsbInterface* intf, void* ctx) {
    UNUSED(intf);
    UNUSED(ctx);

    dap_state.usb_dev = dev;
    if(dap_state.semaphore == NULL) dap_state.semaphore = furi_semaphore_alloc(1, 1);

    usb_hid.dev_descr->idVendor = DAP_HID_VID;
    usb_hid.dev_descr->idProduct = DAP_HID_PID;

    usbd_reg_config(dev, hid_ep_config);
    usbd_reg_control(dev, hid_control);

    usbd_connect(dev, true);
}

static void hid_deinit(usbd_device* dev) {
    dap_state.usb_dev = NULL;

    furi_semaphore_free(dap_state.semaphore);
    dap_state.semaphore = NULL;

    usbd_reg_config(dev, NULL);
    usbd_reg_control(dev, NULL);

    free(usb_hid.str_manuf_descr);
    free(usb_hid.str_prod_descr);
}

static void hid_on_wakeup(usbd_device* dev) {
    UNUSED(dev);
    if(!dap_state.connected) {
        dap_state.connected = true;
        if(dap_state.state_callback != NULL) {
            dap_state.state_callback(dap_state.connected, dap_state.context);
        }
    }
}

static void hid_on_suspend(usbd_device* dev) {
    UNUSED(dev);
    if(dap_state.connected) {
        dap_state.connected = false;
        if(dap_state.state_callback != NULL) {
            dap_state.state_callback(dap_state.connected, dap_state.context);
        }
    }
}

static void hid_txrx_ep_callback(usbd_device* dev, uint8_t event, uint8_t ep) {
    uint8_t data[64];
    int32_t len;

    switch(event) {
    case usbd_evt_eptx:
        furi_semaphore_release(dap_state.semaphore);
        furi_console_log_printf("tx complete");
        break;
    case usbd_evt_eprx:
        len = usbd_ep_read(dev, ep, data, 64);
        furi_console_log_printf("rx %ld", len);

        len = ((len < 0) ? 0 : len);

        if(dap_state.rx_callback != NULL) {
            dap_state.rx_callback(data, len, dap_state.context);
        }
    default:
        break;
    }
}

static usbd_respond hid_ep_config(usbd_device* dev, uint8_t cfg) {
    switch(cfg) {
    case EP_CFG_DECONFIGURE:
        usbd_ep_deconfig(dev, DAP_HID_EP_OUT);
        usbd_ep_deconfig(dev, DAP_HID_EP_IN);
        usbd_reg_endpoint(dev, DAP_HID_EP_OUT, 0);
        usbd_reg_endpoint(dev, DAP_HID_EP_IN, 0);
        return usbd_ack;
    case EP_CFG_CONFIGURE:
        usbd_ep_config(dev, DAP_HID_EP_IN, USB_EPTYPE_INTERRUPT, DAP_HID_EP_SIZE);
        usbd_ep_config(dev, DAP_HID_EP_OUT, USB_EPTYPE_INTERRUPT, DAP_HID_EP_SIZE);
        usbd_reg_endpoint(dev, DAP_HID_EP_IN, hid_txrx_ep_callback);
        usbd_reg_endpoint(dev, DAP_HID_EP_OUT, hid_txrx_ep_callback);
        usbd_ep_write(dev, DAP_HID_EP_IN, 0, 0);
        return usbd_ack;
    default:
        return usbd_fail;
    }
}
static usbd_respond hid_control(usbd_device* dev, usbd_ctlreq* req, usbd_rqc_callback* callback) {
    UNUSED(callback);

    furi_console_log_printf(
        "control: RT %d, R %d, V %d, I %d, L %d",
        req->bmRequestType,
        req->bRequest,
        req->wValue,
        req->wIndex,
        req->wLength);

    if(((USB_REQ_RECIPIENT | USB_REQ_TYPE) & req->bmRequestType) ==
           (USB_REQ_INTERFACE | USB_REQ_CLASS) &&
       req->wIndex == 0) {
        switch(req->bRequest) {
        case USB_HID_GETREPORT:
            furi_console_log_printf("get report");
            return usbd_fail;
        case USB_HID_SETIDLE:
            furi_console_log_printf("set idle");
            return usbd_ack;
        default:
            return usbd_fail;
        }
    }
    if(((USB_REQ_RECIPIENT | USB_REQ_TYPE) & req->bmRequestType) ==
           (USB_REQ_INTERFACE | USB_REQ_STANDARD) &&
       req->wIndex == 0 && req->bRequest == USB_STD_GET_DESCRIPTOR) {
        switch(req->wValue >> 8) {
        case USB_DTYPE_HID:
            furi_console_log_printf("get hid descriptor");
            dev->status.data_ptr = (uint8_t*)&(hid_cfg_desc.hid);
            dev->status.data_count = sizeof(hid_cfg_desc.hid);
            return usbd_ack;
        case USB_DTYPE_HID_REPORT:
            furi_console_log_printf("get hid report descriptor");
            dev->status.data_ptr = (uint8_t*)hid_report_desc;
            dev->status.data_count = sizeof(hid_report_desc);
            return usbd_ack;
        default:
            return usbd_fail;
        }
    }
    return usbd_fail;

    return usbd_fail;
}