#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "lvgl.h"
#include "src/drivers/display/st7796/lv_st7796.h"
#include "src/drivers/display/lcd/lv_lcd_generic_mipi.h"
#include "demos/lv_demos.h"
#include "ft6336u.h"

/*===========================================
 * ST7796 Display Configuration
 *===========================================*/

/* Display resolution - adjust according to your display */
#define DISP_HOR_RES    480
#define DISP_VER_RES    320

/* SPI Configuration */
#define SPI_PORT        spi0
#define SPI_BAUDRATE    (1000 * 1000 * 1000)  /* 75 MHz - maximum for ST7796 */

/* GPIO Pin Configuration - adjust to match your wiring */
#define PIN_SCK         18   /* SPI Clock */
#define PIN_MOSI        19   /* SPI MOSI (TX) */
#define PIN_MISO        16   /* SPI MISO (RX) - not used for display */
#define PIN_CS          17   /* Chip Select */
#define PIN_DC          20   /* Data/Command */
#define PIN_RST         21   /* Reset */
#define PIN_BL          22   /* Backlight (optional) */

/*===========================================
 * FT6336U Touch Configuration
 *===========================================*/

/* I2C Configuration for touch controller */
#define TOUCH_I2C_PORT      i2c1
#define TOUCH_I2C_BAUDRATE  (400 * 1000)  /* 400 kHz */
#define PIN_TOUCH_SDA       2    /* I2C SDA - adjust to your wiring */
#define PIN_TOUCH_SCL       3    /* I2C SCL - adjust to your wiring */
#define PIN_TOUCH_RST       4    /* Touch reset pin (optional, -1 if not used) */
#define PIN_TOUCH_INT       5    /* Touch interrupt pin (optional, -1 if not used) */

/* Touch screen orientation settings */
#define TOUCH_SWAP_XY       true   /* Swap X and Y axes for landscape */
#define TOUCH_INVERT_X      true   /* Invert X axis for landscape */
#define TOUCH_INVERT_Y      false  /* Invert Y axis */

/* FT6336U device handle */
static ft6336u_t touch_dev;

/* Display buffer - Half screen double buffering */
/* 480 x 160 x 2 bytes = 153,600 bytes (150KB) per buffer */
#define DISP_BUF_LINES  160  /* Half screen */
#define DISP_BUF_SIZE   (DISP_HOR_RES * DISP_BUF_LINES)
static uint8_t disp_buf1[DISP_BUF_SIZE * 2] __attribute__((aligned(4)));  /* RGB565 buffer 1 */
static uint8_t disp_buf2[DISP_BUF_SIZE * 2] __attribute__((aligned(4)));  /* RGB565 buffer 2 */

/* DMA channel for SPI transfer */
static int dma_channel;
static volatile bool dma_transfer_done = true;
static lv_display_t *current_disp = NULL;

/*===========================================
 * SPI and GPIO Low-level Functions
 *===========================================*/

static inline void cs_select(void) {
    gpio_put(PIN_CS, 0);
}

static inline void cs_deselect(void) {
    gpio_put(PIN_CS, 1);
}

static inline void dc_command(void) {
    gpio_put(PIN_DC, 0);
}

static inline void dc_data(void) {
    gpio_put(PIN_DC, 1);
}

/* Hardware reset */
static void lcd_reset(void) {
    gpio_put(PIN_RST, 0);
    sleep_ms(100);
    gpio_put(PIN_RST, 1);
    sleep_ms(100);
}

/* DMA interrupt handler */
static void dma_irq_handler(void) {
    if (dma_channel_get_irq0_status(dma_channel)) {
        dma_channel_acknowledge_irq0(dma_channel);
        cs_deselect();
        dma_transfer_done = true;
        
        /* Notify LVGL that flush is complete */
        if (current_disp) {
            lv_display_flush_ready(current_disp);
        }
    }
}

/*===========================================
 * LVGL Display Driver Callbacks
 *===========================================*/

/**
 * Send command to LCD controller (blocking)
 */
static void lcd_send_cmd(lv_display_t *disp, const uint8_t *cmd, size_t cmd_size, 
                         const uint8_t *param, size_t param_size)
{
    (void)disp;
    
    /* Wait for any previous DMA transfer to complete */
    while (!dma_transfer_done) {
        tight_loop_contents();
    }
    
    cs_select();
    
    /* Send command */
    dc_command();
    spi_write_blocking(SPI_PORT, cmd, cmd_size);
    
    /* Send parameters if any */
    if (param_size > 0) {
        dc_data();
        spi_write_blocking(SPI_PORT, param, param_size);
    }
    
    cs_deselect();
}

/**
 * Send pixel data to LCD controller (with DMA)
 */
static void lcd_send_color(lv_display_t *disp, const uint8_t *cmd, size_t cmd_size, 
                           uint8_t *param, size_t param_size)
{
    /* Wait for any previous DMA transfer to complete */
    while (!dma_transfer_done) {
        tight_loop_contents();
    }
    
    current_disp = disp;
    
    /* Swap bytes for RGB565 using optimized 32-bit operations */
    uint32_t *pixels32 = (uint32_t *)param;
    size_t count32 = param_size / 4;
    for (size_t i = 0; i < count32; i++) {
        uint32_t v = pixels32[i];
        /* Swap bytes within each 16-bit word: ABCD -> BADC */
        pixels32[i] = ((v & 0x00FF00FF) << 8) | ((v & 0xFF00FF00) >> 8);
    }
    
    cs_select();
    
    /* Send command (blocking) */
    dc_command();
    spi_write_blocking(SPI_PORT, cmd, cmd_size);
    
    /* Send pixel data using DMA */
    dc_data();
    dma_transfer_done = false;
    
    /* Configure and start DMA with 8-bit transfers */
    dma_channel_config cfg = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&cfg, true);
    channel_config_set_write_increment(&cfg, false);
    channel_config_set_dreq(&cfg, spi_get_dreq(SPI_PORT, true));
    
    dma_channel_configure(dma_channel, &cfg, &spi_get_hw(SPI_PORT)->dr, param, param_size, true);
    
    /* DMA transfer started - will complete in background via interrupt */
}

/*===========================================
 * Tick Callback
 *===========================================*/

static uint32_t lv_tick_cb(void) {
    return to_ms_since_boot(get_absolute_time());
}

/*===========================================
 * Hardware Initialization
 *===========================================*/

static void hardware_init(void) {
    /* Initialize SPI */
    uint actual_baudrate = spi_init(SPI_PORT, SPI_BAUDRATE);
    printf("SPI baudrate: requested %lu MHz, actual %lu MHz\n", 
           SPI_BAUDRATE / 1000000, actual_baudrate / 1000000);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    
    /* Initialize control pins */
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);
    
    gpio_init(PIN_DC);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    
    gpio_init(PIN_RST);
    gpio_set_dir(PIN_RST, GPIO_OUT);
    gpio_put(PIN_RST, 1);
    
    /* Initialize backlight (optional) */
    gpio_init(PIN_BL);
    gpio_set_dir(PIN_BL, GPIO_OUT);
    gpio_put(PIN_BL, 1);  /* Turn on backlight */
    
    /* Initialize I2C for touch controller */
    i2c_init(TOUCH_I2C_PORT, TOUCH_I2C_BAUDRATE);
    gpio_set_function(PIN_TOUCH_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_TOUCH_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_TOUCH_SDA);
    gpio_pull_up(PIN_TOUCH_SCL);
    
    /* Initialize touch reset pin if used */
    #if PIN_TOUCH_RST >= 0
    gpio_init(PIN_TOUCH_RST);
    gpio_set_dir(PIN_TOUCH_RST, GPIO_OUT);
    gpio_put(PIN_TOUCH_RST, 0);
    sleep_ms(10);
    gpio_put(PIN_TOUCH_RST, 1);
    sleep_ms(100);
    #endif
    
    /* Initialize touch interrupt pin if used */
    #if PIN_TOUCH_INT >= 0
    gpio_init(PIN_TOUCH_INT);
    gpio_set_dir(PIN_TOUCH_INT, GPIO_IN);
    #endif
    
    /* Initialize DMA */
    dma_channel = dma_claim_unused_channel(true);
    dma_channel_set_irq0_enabled(dma_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_irq_handler);
    irq_set_enabled(DMA_IRQ_0, true);
    
    /* Hardware reset the display */
    lcd_reset();
}

/*===========================================
 * Touch Input Callback for LVGL
 *===========================================*/

static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;  /* Unused parameter */
    
    uint16_t x, y;
    
    if (ft6336u_read_touch(&touch_dev, &x, &y)) {
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/*===========================================
 * Touch Controller Initialization
 *===========================================*/

static bool touch_init(void)
{
    ft6336u_config_t config = {
        .i2c = TOUCH_I2C_PORT,
        .addr = FT6336U_I2C_ADDR,
        .max_x = DISP_HOR_RES,
        .max_y = DISP_VER_RES,
        .swap_xy = TOUCH_SWAP_XY,
        .invert_x = TOUCH_INVERT_X,
        .invert_y = TOUCH_INVERT_Y,
    };
    
    return ft6336u_init(&touch_dev, &config);
}

/*===========================================
 * Main Function
 *===========================================*/

int main(void)
{
    stdio_init_all();
    
    printf("System clock: %lu MHz\n", clock_get_hz(clk_sys) / 1000000);
    printf("Initializing hardware...\n");
    hardware_init();
    
    printf("Initializing LVGL...\n");
    lv_init();
    lv_tick_set_cb(lv_tick_cb);
    
    printf("Creating ST7796 display...\n");
    
    /* Create ST7796 display */
    lv_display_t *disp = lv_st7796_create(
        DISP_HOR_RES, 
        DISP_VER_RES, 
        LV_LCD_FLAG_BGR,
        lcd_send_cmd, 
        lcd_send_color
    );
    
    /* Set landscape mode: swap X/Y axes and adjust mirroring */
    lv_lcd_generic_mipi_set_address_mode(disp, false, true, true, true);
    
    /* Set display buffer - double buffering for async DMA transfer */
    lv_display_set_buffers(disp, disp_buf1, disp_buf2, sizeof(disp_buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
    
    /* Initialize touch controller */
    printf("Initializing FT6336U touch controller...\n");
    if (touch_init()) {
        /* Create LVGL input device for touch */
        lv_indev_t *indev = lv_indev_create();
        lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(indev, touch_read_cb);
        lv_indev_set_display(indev, disp);
        printf("Touch controller initialized!\n");
    } else {
        printf("Warning: Touch controller initialization failed!\n");
    }
    
    printf("Starting LVGL Benchmark Demo (Single Core)...\n");
    
    /* Run the benchmark demo */
    lv_demo_benchmark();
    
    printf("LVGL initialized! Entering main loop...\n");

    while (true) {
        lv_timer_handler();
        /* No delay - run as fast as possible */
    }
    
    return 0;
}
