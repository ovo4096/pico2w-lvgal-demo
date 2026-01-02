#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "lvgl.h"
#include "src/drivers/display/st7796/lv_st7796.h"

/*===========================================
 * ST7796 Display Configuration
 *===========================================*/

/* Display resolution - adjust according to your display */
#define DISP_HOR_RES    320
#define DISP_VER_RES    480

/* SPI Configuration */
#define SPI_PORT        spi0
#define SPI_BAUDRATE    (40 * 1000 * 1000)  /* 40 MHz */

/* GPIO Pin Configuration - adjust to match your wiring */
#define PIN_SCK         18   /* SPI Clock */
#define PIN_MOSI        19   /* SPI MOSI (TX) */
#define PIN_MISO        16   /* SPI MISO (RX) - not used for display */
#define PIN_CS          17   /* Chip Select */
#define PIN_DC          20   /* Data/Command */
#define PIN_RST         21   /* Reset */
#define PIN_BL          22   /* Backlight (optional) */

/* Display buffer - partial mode with 10 lines */
#define DISP_BUF_SIZE   (DISP_HOR_RES * 10)
static uint8_t disp_buf1[DISP_BUF_SIZE * 2];  /* RGB565 = 2 bytes per pixel */

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
 * Send pixel data to LCD controller (using DMA for async transfer)
 */
static void lcd_send_color(lv_display_t *disp, const uint8_t *cmd, size_t cmd_size, 
                           uint8_t *param, size_t param_size)
{
    /* Wait for any previous DMA transfer to complete */
    while (!dma_transfer_done) {
        tight_loop_contents();
    }
    
    /* Swap bytes for RGB565 - ST7796 expects big-endian but RP2350 is little-endian */
    uint16_t *pixels = (uint16_t *)param;
    size_t pixel_count = param_size / 2;
    for (size_t i = 0; i < pixel_count; i++) {
        pixels[i] = (pixels[i] >> 8) | (pixels[i] << 8);
    }
    
    current_disp = disp;
    
    cs_select();
    
    /* Send command (blocking) */
    dc_command();
    spi_write_blocking(SPI_PORT, cmd, cmd_size);
    
    /* Send pixel data using DMA */
    dc_data();
    dma_transfer_done = false;
    
    dma_channel_configure(
        dma_channel,
        &(dma_channel_config){
            .ctrl = dma_channel_hw_addr(dma_channel)->ctrl_trig
        },
        &spi_get_hw(SPI_PORT)->dr,  /* Destination: SPI data register */
        param,                       /* Source: pixel data */
        param_size,                  /* Transfer count */
        false                        /* Don't start yet */
    );
    
    /* Configure and start DMA */
    dma_channel_config cfg = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&cfg, true);
    channel_config_set_write_increment(&cfg, false);
    channel_config_set_dreq(&cfg, spi_get_dreq(SPI_PORT, true));
    
    dma_channel_configure(dma_channel, &cfg, &spi_get_hw(SPI_PORT)->dr, param, param_size, true);
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
    spi_init(SPI_PORT, SPI_BAUDRATE);
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
    
    /* Initialize DMA */
    dma_channel = dma_claim_unused_channel(true);
    dma_channel_set_irq0_enabled(dma_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_irq_handler);
    irq_set_enabled(DMA_IRQ_0, true);
    
    /* Hardware reset the display */
    lcd_reset();
}

/*===========================================
 * Main Function
 *===========================================*/

int main(void)
{
    stdio_init_all();
    
    printf("Initializing hardware...\n");
    hardware_init();
    
    printf("Initializing LVGL...\n");
    lv_init();
    lv_tick_set_cb(lv_tick_cb);
    
    printf("Creating ST7796 display...\n");
    
    /* Create ST7796 display
     * Color flags to try if colors are wrong:
     * - LV_LCD_FLAG_NONE        : RGB order
     * - LV_LCD_FLAG_BGR         : BGR order  
     * - LV_LCD_FLAG_MIRROR_X    : Mirror horizontally
     * - LV_LCD_FLAG_MIRROR_Y    : Mirror vertically
     * You can combine flags with | operator
     */
    lv_display_t *disp = lv_st7796_create(
        DISP_HOR_RES, 
        DISP_VER_RES, 
        LV_LCD_FLAG_BGR,           /* BGR order - red and blue swapped */
        lcd_send_cmd, 
        lcd_send_color
    );
    
    /* Set color inversion if colors look inverted */
    // lv_st7796_set_invert(disp, true);
    
    /* Set display buffer */
    lv_display_set_buffers(disp, disp_buf1, NULL, sizeof(disp_buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
    
    /* If your display is rotated, you can set rotation here:
     * lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);
     */
    
    printf("Creating UI...\n");
    
    lv_obj_t *scr = lv_screen_active();
    
    /* Set black background */
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    
    /* ====== RGB Color Test Blocks (left top corner) ====== */
    #define COLOR_BLOCK_SIZE 50
    
    /* Red block */
    lv_obj_t *red_block = lv_obj_create(scr);
    lv_obj_set_size(red_block, COLOR_BLOCK_SIZE, COLOR_BLOCK_SIZE);
    lv_obj_set_pos(red_block, 10, 10);
    lv_obj_set_style_bg_color(red_block, lv_color_make(255, 0, 0), 0);
    lv_obj_set_style_border_width(red_block, 0, 0);
    lv_obj_set_style_radius(red_block, 0, 0);
    
    /* Green block */
    lv_obj_t *green_block = lv_obj_create(scr);
    lv_obj_set_size(green_block, COLOR_BLOCK_SIZE, COLOR_BLOCK_SIZE);
    lv_obj_set_pos(green_block, 10 + COLOR_BLOCK_SIZE + 5, 10);
    lv_obj_set_style_bg_color(green_block, lv_color_make(0, 255, 0), 0);
    lv_obj_set_style_border_width(green_block, 0, 0);
    lv_obj_set_style_radius(green_block, 0, 0);
    
    /* Blue block */
    lv_obj_t *blue_block = lv_obj_create(scr);
    lv_obj_set_size(blue_block, COLOR_BLOCK_SIZE, COLOR_BLOCK_SIZE);
    lv_obj_set_pos(blue_block, 10 + (COLOR_BLOCK_SIZE + 5) * 2, 10);
    lv_obj_set_style_bg_color(blue_block, lv_color_make(0, 0, 255), 0);
    lv_obj_set_style_border_width(blue_block, 0, 0);
    lv_obj_set_style_radius(blue_block, 0, 0);
    
    /* Labels under color blocks */
    lv_obj_t *red_label = lv_label_create(scr);
    lv_label_set_text(red_label, "R");
    lv_obj_set_style_text_color(red_label, lv_color_white(), 0);
    lv_obj_align_to(red_label, red_block, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);
    
    lv_obj_t *green_label = lv_label_create(scr);
    lv_label_set_text(green_label, "G");
    lv_obj_set_style_text_color(green_label, lv_color_white(), 0);
    lv_obj_align_to(green_label, green_block, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);
    
    lv_obj_t *blue_label = lv_label_create(scr);
    lv_label_set_text(blue_label, "B");
    lv_obj_set_style_text_color(blue_label, lv_color_white(), 0);
    lv_obj_align_to(blue_label, blue_block, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);
    
    /* ====== Draw red border (1 pixel) ====== */
    
    /* Top border */
    static lv_point_precise_t top_points[] = {{0, 0}, {DISP_HOR_RES - 1, 0}};
    lv_obj_t *top_line = lv_line_create(scr);
    lv_line_set_points(top_line, top_points, 2);
    lv_obj_set_style_line_color(top_line, lv_color_make(255, 0, 0), 0);
    lv_obj_set_style_line_width(top_line, 1, 0);
    
    /* Bottom border */
    static lv_point_precise_t bottom_points[] = {{0, DISP_VER_RES - 1}, {DISP_HOR_RES - 1, DISP_VER_RES - 1}};
    lv_obj_t *bottom_line = lv_line_create(scr);
    lv_line_set_points(bottom_line, bottom_points, 2);
    lv_obj_set_style_line_color(bottom_line, lv_color_make(255, 0, 0), 0);
    lv_obj_set_style_line_width(bottom_line, 1, 0);
    
    /* Left border */
    static lv_point_precise_t left_points[] = {{0, 0}, {0, DISP_VER_RES - 1}};
    lv_obj_t *left_line = lv_line_create(scr);
    lv_line_set_points(left_line, left_points, 2);
    lv_obj_set_style_line_color(left_line, lv_color_make(255, 0, 0), 0);
    lv_obj_set_style_line_width(left_line, 1, 0);
    
    /* Right border */
    static lv_point_precise_t right_points[] = {{DISP_HOR_RES - 1, 0}, {DISP_HOR_RES - 1, DISP_VER_RES - 1}};
    lv_obj_t *right_line = lv_line_create(scr);
    lv_line_set_points(right_line, right_points, 2);
    lv_obj_set_style_line_color(right_line, lv_color_make(255, 0, 0), 0);
    lv_obj_set_style_line_width(right_line, 1, 0);
    
    /* ====== Draw diagonal lines ====== */
    
    /* Diagonal: top-left to bottom-right */
    static lv_point_precise_t diag1_points[] = {{0, 0}, {DISP_HOR_RES - 1, DISP_VER_RES - 1}};
    lv_obj_t *diag1_line = lv_line_create(scr);
    lv_line_set_points(diag1_line, diag1_points, 2);
    lv_obj_set_style_line_color(diag1_line, lv_color_make(255, 0, 0), 0);
    lv_obj_set_style_line_width(diag1_line, 1, 0);
    
    /* Diagonal: top-right to bottom-left */
    static lv_point_precise_t diag2_points[] = {{DISP_HOR_RES - 1, 0}, {0, DISP_VER_RES - 1}};
    lv_obj_t *diag2_line = lv_line_create(scr);
    lv_line_set_points(diag2_line, diag2_points, 2);
    lv_obj_set_style_line_color(diag2_line, lv_color_make(255, 0, 0), 0);
    lv_obj_set_style_line_width(diag2_line, 1, 0);
    
    /* ====== Center label ====== */
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "320x480");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    
    printf("LVGL initialized! Entering main loop...\n");

    while (true) {
        lv_timer_handler();
        sleep_ms(5);
    }
    
    return 0;
}
