/**
 * @file ft6336u.h
 * @brief FT6336U Capacitive Touch Controller Driver for Raspberry Pi Pico
 * 
 * This driver interfaces with the FT6336U touch controller via I2C
 * and provides touch input for LVGL integration.
 */

#ifndef FT6336U_H
#define FT6336U_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"

/*===========================================
 * FT6336U Configuration
 *===========================================*/

/* FT6336U I2C Address (7-bit) */
#define FT6336U_I2C_ADDR        0x38

/* FT6336U Register Addresses */
#define FT6336U_REG_DEV_MODE    0x00    /* Device mode */
#define FT6336U_REG_GEST_ID     0x01    /* Gesture ID */
#define FT6336U_REG_TD_STATUS   0x02    /* Touch point status */
#define FT6336U_REG_P1_XH       0x03    /* Touch point 1 X high */
#define FT6336U_REG_P1_XL       0x04    /* Touch point 1 X low */
#define FT6336U_REG_P1_YH       0x05    /* Touch point 1 Y high */
#define FT6336U_REG_P1_YL       0x06    /* Touch point 1 Y low */
#define FT6336U_REG_P1_WEIGHT   0x07    /* Touch point 1 weight */
#define FT6336U_REG_P1_MISC     0x08    /* Touch point 1 misc */
#define FT6336U_REG_P2_XH       0x09    /* Touch point 2 X high */
#define FT6336U_REG_P2_XL       0x0A    /* Touch point 2 X low */
#define FT6336U_REG_P2_YH       0x0B    /* Touch point 2 Y high */
#define FT6336U_REG_P2_YL       0x0C    /* Touch point 2 Y low */
#define FT6336U_REG_P2_WEIGHT   0x0D    /* Touch point 2 weight */
#define FT6336U_REG_P2_MISC     0x0E    /* Touch point 2 misc */
#define FT6336U_REG_TH_GROUP    0x80    /* Threshold for touch detection */
#define FT6336U_REG_TH_DIFF     0x85    /* Filter function coefficient */
#define FT6336U_REG_CTRL        0x86    /* Control register */
#define FT6336U_REG_TIMEENTER   0x87    /* Time to enter monitor mode */
#define FT6336U_REG_PERIODACTIVE 0x88   /* Period of active status */
#define FT6336U_REG_PERIODMONITOR 0x89  /* Period of monitor status */
#define FT6336U_REG_RADIAN_VALUE 0x91   /* Minimum radian for gesture */
#define FT6336U_REG_OFFSET_UD   0x92    /* Offset up-down */
#define FT6336U_REG_OFFSET_LR   0x93    /* Offset left-right */
#define FT6336U_REG_DIST_UD     0x94    /* Distance up-down */
#define FT6336U_REG_DIST_LR     0x95    /* Distance left-right */
#define FT6336U_REG_DIST_ZOOM   0x96    /* Distance zoom */
#define FT6336U_REG_LIB_VER_H   0xA1    /* Library version high byte */
#define FT6336U_REG_LIB_VER_L   0xA2    /* Library version low byte */
#define FT6336U_REG_CIPHER      0xA3    /* Chip selecting */
#define FT6336U_REG_G_MODE      0xA4    /* Interrupt mode */
#define FT6336U_REG_PWR_MODE    0xA5    /* Power mode */
#define FT6336U_REG_FIRMID      0xA6    /* Firmware version */
#define FT6336U_REG_FOCALTECH_ID 0xA8   /* Focaltech's panel ID */
#define FT6336U_REG_RELEASE_CODE 0xAF   /* Release code version */
#define FT6336U_REG_STATE       0xBC    /* Current operating mode */

/* Touch event types */
#define FT6336U_EVENT_PRESS_DOWN    0x00
#define FT6336U_EVENT_LIFT_UP       0x01
#define FT6336U_EVENT_CONTACT       0x02
#define FT6336U_EVENT_NO_EVENT      0x03

/* Gesture IDs */
#define FT6336U_GEST_MOVE_UP        0x10
#define FT6336U_GEST_MOVE_RIGHT     0x14
#define FT6336U_GEST_MOVE_DOWN      0x18
#define FT6336U_GEST_MOVE_LEFT      0x1C
#define FT6336U_GEST_ZOOM_IN        0x48
#define FT6336U_GEST_ZOOM_OUT       0x49
#define FT6336U_GEST_NONE           0x00

/* Interrupt modes */
#define FT6336U_G_MODE_POLLING      0x00
#define FT6336U_G_MODE_TRIGGER      0x01

/* Maximum touch points */
#define FT6336U_MAX_TOUCH_POINTS    2

/*===========================================
 * Data Structures
 *===========================================*/

/**
 * @brief Touch point data structure
 */
typedef struct {
    uint16_t x;         /* X coordinate */
    uint16_t y;         /* Y coordinate */
    uint8_t event;      /* Event type (press, release, contact) */
    uint8_t weight;     /* Touch pressure/weight */
    uint8_t area;       /* Touch area */
    bool valid;         /* Whether this touch point is valid */
} ft6336u_touch_point_t;

/**
 * @brief Touch data structure (for multi-touch)
 */
typedef struct {
    uint8_t gesture_id;                             /* Gesture ID */
    uint8_t touch_count;                            /* Number of touch points detected */
    ft6336u_touch_point_t points[FT6336U_MAX_TOUCH_POINTS];  /* Touch points */
} ft6336u_touch_data_t;

/**
 * @brief FT6336U device configuration
 */
typedef struct {
    i2c_inst_t *i2c;        /* I2C instance (i2c0 or i2c1) */
    uint8_t addr;           /* I2C address (default: 0x38) */
    uint16_t max_x;         /* Maximum X coordinate (display width) */
    uint16_t max_y;         /* Maximum Y coordinate (display height) */
    bool swap_xy;           /* Swap X and Y coordinates */
    bool invert_x;          /* Invert X coordinate */
    bool invert_y;          /* Invert Y coordinate */
} ft6336u_config_t;

/**
 * @brief FT6336U device handle
 */
typedef struct {
    ft6336u_config_t config;
    bool initialized;
    ft6336u_touch_data_t last_touch;
} ft6336u_t;

/*===========================================
 * Function Prototypes
 *===========================================*/

/**
 * @brief Initialize the FT6336U touch controller
 * @param dev Pointer to device handle
 * @param config Device configuration
 * @return true on success, false on failure
 */
bool ft6336u_init(ft6336u_t *dev, const ft6336u_config_t *config);

/**
 * @brief Read touch data from the controller
 * @param dev Pointer to device handle
 * @param data Pointer to touch data structure to fill
 * @return true if touch is detected, false otherwise
 */
bool ft6336u_read(ft6336u_t *dev, ft6336u_touch_data_t *data);

/**
 * @brief Read single touch point (convenience function)
 * @param dev Pointer to device handle
 * @param x Pointer to store X coordinate
 * @param y Pointer to store Y coordinate
 * @return true if touch is detected, false otherwise
 */
bool ft6336u_read_touch(ft6336u_t *dev, uint16_t *x, uint16_t *y);

/**
 * @brief Check if touch is currently pressed
 * @param dev Pointer to device handle
 * @return true if touched, false otherwise
 */
bool ft6336u_is_touched(ft6336u_t *dev);

/**
 * @brief Get firmware version
 * @param dev Pointer to device handle
 * @return Firmware version or 0 on error
 */
uint8_t ft6336u_get_firmware_version(ft6336u_t *dev);

/**
 * @brief Get library version
 * @param dev Pointer to device handle
 * @return Library version (16-bit) or 0 on error
 */
uint16_t ft6336u_get_library_version(ft6336u_t *dev);

/**
 * @brief Get vendor ID
 * @param dev Pointer to device handle
 * @return Vendor ID or 0 on error
 */
uint8_t ft6336u_get_vendor_id(ft6336u_t *dev);

/**
 * @brief Set touch detection threshold
 * @param dev Pointer to device handle
 * @param threshold Touch threshold value (0-255)
 * @return true on success, false on failure
 */
bool ft6336u_set_threshold(ft6336u_t *dev, uint8_t threshold);

/**
 * @brief Set interrupt mode
 * @param dev Pointer to device handle
 * @param mode Interrupt mode (polling or trigger)
 * @return true on success, false on failure
 */
bool ft6336u_set_interrupt_mode(ft6336u_t *dev, uint8_t mode);

#endif /* FT6336U_H */
