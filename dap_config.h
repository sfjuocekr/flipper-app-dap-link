// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022, Alex Taradov <alex@taradov.com>. All rights reserved.

#ifndef _DAP_CONFIG_H_
#define _DAP_CONFIG_H_

/*- Includes ----------------------------------------------------------------*/
#include <furi_hal_gpio.h>
#include <furi_hal_resources.h>

/*- Definitions -------------------------------------------------------------*/
// #define DAP_CONFIG_ENABLE_JTAG

#define DAP_CONFIG_DEFAULT_PORT DAP_PORT_SWD
#define DAP_CONFIG_DEFAULT_CLOCK 1000000 // Hz

#define DAP_CONFIG_PACKET_SIZE 64
#define DAP_CONFIG_PACKET_COUNT 1

#define DAP_CONFIG_JTAG_DEV_COUNT 8

// DAP_CONFIG_PRODUCT_STR must contain "CMSIS-DAP" to be compatible with the standard
#define DAP_CONFIG_VENDOR_STR "Flipper Zero"
#define DAP_CONFIG_PRODUCT_STR "Generic CMSIS-DAP Adapter"
#define DAP_CONFIG_SER_NUM_STR usb_serial_number
#define DAP_CONFIG_CMSIS_DAP_VER_STR "2.0.0"

//#define DAP_CONFIG_RESET_TARGET_FN     target_specific_reset_function
//#define DAP_CONFIG_VENDOR_FN           vendor_command_handler_function

// Attribute to use for performance-critical functions
#define DAP_CONFIG_PERFORMANCE_ATTR

// A value at which dap_clock_test() produces 1 kHz output on the SWCLK pin
#define DAP_CONFIG_DELAY_CONSTANT 19000

// A threshold for switching to fast clock (no added delays)
// This is the frequency produced by dap_clock_test(1) on the SWCLK pin
#define DAP_CONFIG_FAST_CLOCK 2400000 // Hz

/*- Prototypes --------------------------------------------------------------*/
extern char usb_serial_number[16];

/*- Implementations ---------------------------------------------------------*/
#define FLIPPER_SWCLK_PIN gpio_ext_pa7
#define FLIPPER_SWDIO_PIN gpio_ext_pa6
#define FLIPPER_RESET_PIN gpio_ext_pa4

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_SWCLK_TCK_write(int value) {
    furi_hal_gpio_write(&FLIPPER_SWCLK_PIN, value);
}

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_SWDIO_TMS_write(int value) {
    furi_hal_gpio_write(&FLIPPER_SWDIO_PIN, value);
}

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_TDI_write(int value) {
#ifdef DAP_CONFIG_ENABLE_JTAG
    HAL_GPIO_TDI_write(value);
#else
    (void)value;
#endif
}

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_TDO_write(int value) {
#ifdef DAP_CONFIG_ENABLE_JTAG
    HAL_GPIO_TDO_write(value);
#else
    (void)value;
#endif
}

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_nTRST_write(int value) {
    (void)value;
}

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_nRESET_write(int value) {
    furi_hal_gpio_write(&FLIPPER_RESET_PIN, value);
}

//-----------------------------------------------------------------------------
static inline int DAP_CONFIG_SWCLK_TCK_read(void) {
    return furi_hal_gpio_read(&FLIPPER_SWCLK_PIN);
}

//-----------------------------------------------------------------------------
static inline int DAP_CONFIG_SWDIO_TMS_read(void) {
    return furi_hal_gpio_read(&FLIPPER_SWDIO_PIN);
}

//-----------------------------------------------------------------------------
static inline int DAP_CONFIG_TDO_read(void) {
#ifdef DAP_CONFIG_ENABLE_JTAG
    return HAL_GPIO_TDO_read();
#else
    return 0;
#endif
}

//-----------------------------------------------------------------------------
static inline int DAP_CONFIG_TDI_read(void) {
#ifdef DAP_CONFIG_ENABLE_JTAG
    return HAL_GPIO_TDI_read();
#else
    return 0;
#endif
}

//-----------------------------------------------------------------------------
static inline int DAP_CONFIG_nTRST_read(void) {
    return 0;
}

//-----------------------------------------------------------------------------
static inline int DAP_CONFIG_nRESET_read(void) {
    return furi_hal_gpio_read(&FLIPPER_RESET_PIN);
}

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_SWCLK_TCK_set(void) {
    LL_GPIO_SetOutputPin(FLIPPER_SWCLK_PIN.port, FLIPPER_SWCLK_PIN.pin);
}

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_SWCLK_TCK_clr(void) {
    LL_GPIO_ResetOutputPin(FLIPPER_SWCLK_PIN.port, FLIPPER_SWCLK_PIN.pin);
}

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_SWDIO_TMS_in(void) {
    LL_GPIO_SetPinMode(FLIPPER_SWDIO_PIN.port, FLIPPER_SWDIO_PIN.pin, LL_GPIO_MODE_INPUT);
}

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_SWDIO_TMS_out(void) {
    LL_GPIO_SetPinMode(FLIPPER_SWDIO_PIN.port, FLIPPER_SWDIO_PIN.pin, LL_GPIO_MODE_OUTPUT);
}

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_SETUP(void) {
    furi_hal_gpio_init(&FLIPPER_SWDIO_PIN, GpioModeInput, GpioPullNo, GpioSpeedVeryHigh);
    furi_hal_gpio_init(&FLIPPER_SWCLK_PIN, GpioModeInput, GpioPullNo, GpioSpeedVeryHigh);
    furi_hal_gpio_init(&FLIPPER_RESET_PIN, GpioModeInput, GpioPullNo, GpioSpeedVeryHigh);
#ifdef DAP_CONFIG_ENABLE_JTAG
    HAL_GPIO_TDO_in();
    HAL_GPIO_TDI_in();
#endif
}

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_DISCONNECT(void) {
    furi_hal_gpio_init(&FLIPPER_SWDIO_PIN, GpioModeInput, GpioPullNo, GpioSpeedVeryHigh);
    furi_hal_gpio_init(&FLIPPER_SWCLK_PIN, GpioModeInput, GpioPullNo, GpioSpeedVeryHigh);
    furi_hal_gpio_init(&FLIPPER_RESET_PIN, GpioModeInput, GpioPullNo, GpioSpeedVeryHigh);
#ifdef DAP_CONFIG_ENABLE_JTAG
    HAL_GPIO_TDO_in();
    HAL_GPIO_TDI_in();
#endif
}

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_CONNECT_SWD(void) {
    furi_hal_gpio_init(&FLIPPER_SWDIO_PIN, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);
    furi_hal_gpio_write(&FLIPPER_SWDIO_PIN, true);

    furi_hal_gpio_init(&FLIPPER_SWCLK_PIN, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);
    furi_hal_gpio_write(&FLIPPER_SWCLK_PIN, true);

    furi_hal_gpio_init(&FLIPPER_RESET_PIN, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);
    furi_hal_gpio_write(&FLIPPER_RESET_PIN, true);

#ifdef DAP_CONFIG_ENABLE_JTAG
    HAL_GPIO_TDO_in();
    HAL_GPIO_TDI_in();
#endif
}

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_CONNECT_JTAG(void) {
    furi_hal_gpio_init(&FLIPPER_SWDIO_PIN, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);
    furi_hal_gpio_write(&FLIPPER_SWDIO_PIN, true);

    furi_hal_gpio_init(&FLIPPER_SWCLK_PIN, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);
    furi_hal_gpio_write(&FLIPPER_SWCLK_PIN, true);

    furi_hal_gpio_init(&FLIPPER_RESET_PIN, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);
    furi_hal_gpio_write(&FLIPPER_RESET_PIN, true);

#ifdef DAP_CONFIG_ENABLE_JTAG
    HAL_GPIO_TDO_in();

    HAL_GPIO_TDI_out();
    HAL_GPIO_TDI_set();
#endif
}

//-----------------------------------------------------------------------------
static inline void DAP_CONFIG_LED(int index, int state) {
    (void)index;
    (void)state;
}

//-----------------------------------------------------------------------------
__attribute__((always_inline)) static inline void DAP_CONFIG_DELAY(uint32_t cycles) {
    asm volatile("1: subs %[cycles], %[cycles], #1 \n"
                 "   bne 1b \n"
                 : [cycles] "+l"(cycles));
}

#endif // _DAP_CONFIG_H_
