/**
 * @file ft6336u.c
 * @brief FT6336U Capacitive Touch Controller Driver Implementation
 */

#include "ft6336u.h"
#include <stdio.h>
#include <string.h>

/*===========================================
 * Private Functions
 *===========================================*/

/**
 * @brief Read a single register from FT6336U
 */
static bool ft6336u_read_reg(ft6336u_t *dev, uint8_t reg, uint8_t *data) {
    int ret;
    
    /* Write register address */
    ret = i2c_write_blocking(dev->config.i2c, dev->config.addr, &reg, 1, true);
    if (ret != 1) {
        return false;
    }
    
    /* Read register value */
    ret = i2c_read_blocking(dev->config.i2c, dev->config.addr, data, 1, false);
    if (ret != 1) {
        return false;
    }
    
    return true;
}

/**
 * @brief Read multiple registers from FT6336U
 */
static bool ft6336u_read_regs(ft6336u_t *dev, uint8_t start_reg, uint8_t *data, size_t len) {
    int ret;
    
    /* Write starting register address */
    ret = i2c_write_blocking(dev->config.i2c, dev->config.addr, &start_reg, 1, true);
    if (ret != 1) {
        return false;
    }
    
    /* Read register values */
    ret = i2c_read_blocking(dev->config.i2c, dev->config.addr, data, len, false);
    if (ret != (int)len) {
        return false;
    }
    
    return true;
}

/**
 * @brief Write a single register to FT6336U
 */
static bool ft6336u_write_reg(ft6336u_t *dev, uint8_t reg, uint8_t data) {
    uint8_t buf[2] = {reg, data};
    int ret;
    
    ret = i2c_write_blocking(dev->config.i2c, dev->config.addr, buf, 2, false);
    if (ret != 2) {
        return false;
    }
    
    return true;
}

/**
 * @brief Apply coordinate transformations
 */
static void ft6336u_transform_coords(ft6336u_t *dev, uint16_t *x, uint16_t *y) {
    uint16_t tx = *x;
    uint16_t ty = *y;
    
    /* Swap X and Y if configured */
    if (dev->config.swap_xy) {
        uint16_t temp = tx;
        tx = ty;
        ty = temp;
    }
    
    /* Invert X if configured */
    if (dev->config.invert_x) {
        tx = dev->config.max_x - 1 - tx;
    }
    
    /* Invert Y if configured */
    if (dev->config.invert_y) {
        ty = dev->config.max_y - 1 - ty;
    }
    
    /* Clamp to valid range */
    if (tx >= dev->config.max_x) tx = dev->config.max_x - 1;
    if (ty >= dev->config.max_y) ty = dev->config.max_y - 1;
    
    *x = tx;
    *y = ty;
}

/*===========================================
 * Public Functions
 *===========================================*/

bool ft6336u_init(ft6336u_t *dev, const ft6336u_config_t *config) {
    if (!dev || !config || !config->i2c) {
        return false;
    }
    
    /* Copy configuration */
    memcpy(&dev->config, config, sizeof(ft6336u_config_t));
    
    /* Set default I2C address if not specified */
    if (dev->config.addr == 0) {
        dev->config.addr = FT6336U_I2C_ADDR;
    }
    
    /* Clear touch data */
    memset(&dev->last_touch, 0, sizeof(dev->last_touch));
    
    /* Try to read device ID to verify communication */
    uint8_t chip_id = 0;
    if (!ft6336u_read_reg(dev, FT6336U_REG_FOCALTECH_ID, &chip_id)) {
        printf("FT6336U: Failed to read chip ID\n");
        return false;
    }
    
    printf("FT6336U: Chip ID = 0x%02X\n", chip_id);
    
    /* Read firmware version */
    uint8_t fw_ver = ft6336u_get_firmware_version(dev);
    printf("FT6336U: Firmware version = 0x%02X\n", fw_ver);
    
    /* Read library version */
    uint16_t lib_ver = ft6336u_get_library_version(dev);
    printf("FT6336U: Library version = 0x%04X\n", lib_ver);
    
    /* Set device to normal operating mode */
    ft6336u_write_reg(dev, FT6336U_REG_DEV_MODE, 0x00);
    
    /* Set touch threshold (optional, adjust as needed) */
    ft6336u_set_threshold(dev, 22);
    
    /* Set interrupt mode to polling */
    ft6336u_set_interrupt_mode(dev, FT6336U_G_MODE_POLLING);
    
    dev->initialized = true;
    printf("FT6336U: Initialized successfully\n");
    
    return true;
}

bool ft6336u_read(ft6336u_t *dev, ft6336u_touch_data_t *data) {
    if (!dev || !dev->initialized || !data) {
        return false;
    }
    
    /* Clear output data */
    memset(data, 0, sizeof(ft6336u_touch_data_t));
    
    /* Read all touch registers in one transaction (more efficient) */
    uint8_t buf[15];  /* Registers 0x00-0x0E */
    if (!ft6336u_read_regs(dev, FT6336U_REG_DEV_MODE, buf, sizeof(buf))) {
        return false;
    }
    
    /* Parse gesture ID */
    data->gesture_id = buf[FT6336U_REG_GEST_ID];
    
    /* Parse touch count (bits 3:0 of TD_STATUS) */
    data->touch_count = buf[FT6336U_REG_TD_STATUS] & 0x0F;
    
    /* Clamp touch count to max supported */
    if (data->touch_count > FT6336U_MAX_TOUCH_POINTS) {
        data->touch_count = FT6336U_MAX_TOUCH_POINTS;
    }
    
    /* Parse touch point 1 */
    if (data->touch_count >= 1) {
        data->points[0].event = (buf[FT6336U_REG_P1_XH] >> 6) & 0x03;
        data->points[0].x = ((buf[FT6336U_REG_P1_XH] & 0x0F) << 8) | buf[FT6336U_REG_P1_XL];
        data->points[0].y = ((buf[FT6336U_REG_P1_YH] & 0x0F) << 8) | buf[FT6336U_REG_P1_YL];
        data->points[0].weight = buf[FT6336U_REG_P1_WEIGHT];
        data->points[0].area = (buf[FT6336U_REG_P1_MISC] >> 4) & 0x0F;
        data->points[0].valid = true;
        
        /* Apply coordinate transformation */
        ft6336u_transform_coords(dev, &data->points[0].x, &data->points[0].y);
    }
    
    /* Parse touch point 2 */
    if (data->touch_count >= 2) {
        data->points[1].event = (buf[FT6336U_REG_P2_XH] >> 6) & 0x03;
        data->points[1].x = ((buf[FT6336U_REG_P2_XH] & 0x0F) << 8) | buf[FT6336U_REG_P2_XL];
        data->points[1].y = ((buf[FT6336U_REG_P2_YH] & 0x0F) << 8) | buf[FT6336U_REG_P2_YL];
        data->points[1].weight = buf[FT6336U_REG_P2_WEIGHT];
        data->points[1].area = (buf[FT6336U_REG_P2_MISC] >> 4) & 0x0F;
        data->points[1].valid = true;
        
        /* Apply coordinate transformation */
        ft6336u_transform_coords(dev, &data->points[1].x, &data->points[1].y);
    }
    
    /* Store last touch data */
    memcpy(&dev->last_touch, data, sizeof(ft6336u_touch_data_t));
    
    return data->touch_count > 0;
}

bool ft6336u_read_touch(ft6336u_t *dev, uint16_t *x, uint16_t *y) {
    ft6336u_touch_data_t data;
    
    if (!ft6336u_read(dev, &data)) {
        return false;
    }
    
    if (data.touch_count > 0 && data.points[0].valid) {
        if (x) *x = data.points[0].x;
        if (y) *y = data.points[0].y;
        return true;
    }
    
    return false;
}

bool ft6336u_is_touched(ft6336u_t *dev) {
    if (!dev || !dev->initialized) {
        return false;
    }
    
    uint8_t status;
    if (!ft6336u_read_reg(dev, FT6336U_REG_TD_STATUS, &status)) {
        return false;
    }
    
    return (status & 0x0F) > 0;
}

uint8_t ft6336u_get_firmware_version(ft6336u_t *dev) {
    uint8_t version = 0;
    ft6336u_read_reg(dev, FT6336U_REG_FIRMID, &version);
    return version;
}

uint16_t ft6336u_get_library_version(ft6336u_t *dev) {
    uint8_t ver_h = 0, ver_l = 0;
    ft6336u_read_reg(dev, FT6336U_REG_LIB_VER_H, &ver_h);
    ft6336u_read_reg(dev, FT6336U_REG_LIB_VER_L, &ver_l);
    return (ver_h << 8) | ver_l;
}

uint8_t ft6336u_get_vendor_id(ft6336u_t *dev) {
    uint8_t vendor_id = 0;
    ft6336u_read_reg(dev, FT6336U_REG_FOCALTECH_ID, &vendor_id);
    return vendor_id;
}

bool ft6336u_set_threshold(ft6336u_t *dev, uint8_t threshold) {
    return ft6336u_write_reg(dev, FT6336U_REG_TH_GROUP, threshold);
}

bool ft6336u_set_interrupt_mode(ft6336u_t *dev, uint8_t mode) {
    return ft6336u_write_reg(dev, FT6336U_REG_G_MODE, mode);
}
