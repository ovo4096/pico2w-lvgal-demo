/**
 * @file lv_conf.h
 * Configuration file for LVGL v9.4.0
 * Configured for Raspberry Pi Pico 2 W
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/

/** Color depth: 1 (I1), 8 (L8), 16 (RGB565), 24 (RGB888), 32 (XRGB8888) */
#define LV_COLOR_DEPTH 16

/*=========================
   STDLIB WRAPPER SETTINGS
 *=========================*/

#define LV_USE_STDLIB_MALLOC    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_BUILTIN

#define LV_STDINT_INCLUDE       <stdint.h>
#define LV_STDDEF_INCLUDE       <stddef.h>
#define LV_STDBOOL_INCLUDE      <stdbool.h>
#define LV_INTTYPES_INCLUDE     <inttypes.h>
#define LV_LIMITS_INCLUDE       <limits.h>
#define LV_STDARG_INCLUDE       <stdarg.h>

/** Size of memory available for `lv_malloc()` in bytes (>= 2kB)
 *  Increased to 128KB for benchmark demo (recommended minimum) */
#define LV_MEM_SIZE (128 * 1024U)

/** Size of the memory expand for `lv_malloc()` in bytes */
#define LV_MEM_POOL_EXPAND_SIZE 0

/** Set an address for the memory pool instead of allocating it as a normal array. */
#define LV_MEM_ADR 0

/*====================
   HAL SETTINGS
 *====================*/

/** Default display refresh, input device read and animation step period. */
#define LV_DEF_REFR_PERIOD  10      /**< [ms] - 100 FPS max */

/** Default Dots Per Inch. */
#define LV_DPI_DEF 130

/*=================
 * OPERATING SYSTEM
 *=================*/

#define LV_USE_OS   LV_OS_NONE

/*========================
 * RENDERING CONFIGURATION
 *========================*/

#define LV_DRAW_BUF_STRIDE_ALIGN                1
#define LV_DRAW_BUF_ALIGN                       4
#define LV_DRAW_TRANSFORM_USE_MATRIX            0

/** The target buffer size for simple layer chunks. */
#define LV_DRAW_LAYER_SIMPLE_BUF_SIZE    (8 * 1024)

#define LV_DRAW_LAYER_MAX_MEMORY 0

#define LV_DRAW_THREAD_STACK_SIZE    (8 * 1024)

#define LV_USE_DRAW_SW 1
#if LV_USE_DRAW_SW == 1
    #define LV_DRAW_SW_SUPPORT_RGB565           1
    #define LV_DRAW_SW_SUPPORT_RGB565_SWAPPED   1
    #define LV_DRAW_SW_SUPPORT_RGB565A8         0
    #define LV_DRAW_SW_SUPPORT_RGB888           0
    #define LV_DRAW_SW_SUPPORT_XRGB8888         0
    #define LV_DRAW_SW_SUPPORT_ARGB8888         1
    #define LV_DRAW_SW_SUPPORT_ARGB8888_PREMULTIPLIED 1
    #define LV_DRAW_SW_SUPPORT_L8               1
    #define LV_DRAW_SW_SUPPORT_AL88             0
    #define LV_DRAW_SW_SUPPORT_A8               1
    #define LV_DRAW_SW_SUPPORT_I1               1
    #define LV_DRAW_SW_I1_LUM_THRESHOLD         127
    #define LV_DRAW_SW_DRAW_UNIT_CNT            1
    #define LV_USE_DRAW_ARM2D_SYNC              0
    #define LV_USE_NATIVE_HELIUM_ASM            0
    #define LV_DRAW_SW_COMPLEX                  1
    #if LV_DRAW_SW_COMPLEX == 1
        #define LV_DRAW_SW_SHADOW_CACHE_SIZE    0
        #define LV_DRAW_SW_CIRCLE_CACHE_SIZE    4
    #endif
    #define LV_USE_DRAW_SW_ASM                  LV_DRAW_SW_ASM_NONE
    #define LV_USE_DRAW_SW_COMPLEX_GRADIENTS    0
#endif

/* Disable unused GPU backends */
#define LV_USE_NEMA_GFX         0
#define LV_USE_PXP              0
#define LV_USE_G2D              0
#define LV_USE_DRAW_DAVE2D      0
#define LV_USE_DRAW_SDL         0
#define LV_USE_DRAW_VG_LITE     0
#define LV_USE_DRAW_DMA2D       0
#define LV_USE_DRAW_OPENGLES    0
#define LV_USE_PPA              0
#define LV_USE_DRAW_EVE         0

/*=======================
 * FEATURE CONFIGURATION
 *=======================*/

/*-------------
 * Logging
 *-----------*/

/** Enable the log module */
#define LV_USE_LOG 0
#if LV_USE_LOG
    #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
    #define LV_LOG_PRINTF 1
    #define LV_LOG_USE_TIMESTAMP 1
    #define LV_LOG_USE_FILE_LINE 1
#endif

/*-------------
 * Asserts
 *-----------*/

#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0

/*-------------
 * Debug
 *-----------*/

#define LV_USE_REFR_DEBUG       0
#define LV_USE_LAYER_DEBUG      0
#define LV_USE_PARALLEL_DRAW_DEBUG 0

/*-------------
 * Others
 *-----------*/

#define LV_ENABLE_GLOBAL_CUSTOM 0

/** Default image cache size. 0 to disable caching. */
#define LV_CACHE_DEF_SIZE       0

/** Default image header cache count. 0 to disable caching. */
#define LV_IMAGE_HEADER_CACHE_DEF_CNT 0

/** Number of gradients that can be stored in RAM. */
#define LV_GRADIENT_MAX_STOPS   2

/** Adjust color mix functions rounding */
#define LV_COLOR_MIX_ROUND_OFS  0

/** Add 2 x 32 bit variables to each lv_obj_t */
#define LV_USE_OBJ_ID           0

/** Use lvgl builtin method for obj ID */
#define LV_USE_OBJ_ID_BUILTIN   0

/** Use obj property set/get API */
#define LV_USE_OBJ_PROPERTY     0

#define LV_USE_OBJ_PROPERTY_NAME 0

/** VG-Lite Simulator */
#define LV_USE_VG_LITE_THORVG   0

/** ThorVG */
#define LV_USE_THORVG_INTERNAL  0
#define LV_USE_THORVG_EXTERNAL  0

/** Use Matrix for transformations. */
#define LV_USE_MATRIX           0

/*=====================
 *  COMPILER SETTINGS
 *====================*/

#define LV_BIG_ENDIAN_SYSTEM    0
#define LV_ATTRIBUTE_TICK_INC
#define LV_ATTRIBUTE_TIMER_HANDLER
#define LV_ATTRIBUTE_FLUSH_READY
#define LV_ATTRIBUTE_MEM_ALIGN_SIZE 1
#define LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_LARGE_CONST
#define LV_ATTRIBUTE_LARGE_RAM_ARRAY
#define LV_ATTRIBUTE_FAST_MEM
#define LV_ATTRIBUTE_DMA
#define LV_EXPORT_CONST_INT(int_value) struct _silence_gcc_warning
#define LV_ATTRIBUTE_EXTERN_DATA

#define LV_USE_FLOAT            0

/*==================
 *   FONT USAGE
 *===================*/

/* Montserrat fonts with ASCII range and some symbols using bpp = 4 */
#define LV_FONT_MONTSERRAT_8    0
#define LV_FONT_MONTSERRAT_10   0
#define LV_FONT_MONTSERRAT_12   1
#define LV_FONT_MONTSERRAT_14   1
#define LV_FONT_MONTSERRAT_16   0
#define LV_FONT_MONTSERRAT_18   0
#define LV_FONT_MONTSERRAT_20   1
#define LV_FONT_MONTSERRAT_22   0
#define LV_FONT_MONTSERRAT_24   1
#define LV_FONT_MONTSERRAT_26   1
#define LV_FONT_MONTSERRAT_28   0
#define LV_FONT_MONTSERRAT_30   0
#define LV_FONT_MONTSERRAT_32   0
#define LV_FONT_MONTSERRAT_34   0
#define LV_FONT_MONTSERRAT_36   0
#define LV_FONT_MONTSERRAT_38   0
#define LV_FONT_MONTSERRAT_40   0
#define LV_FONT_MONTSERRAT_42   0
#define LV_FONT_MONTSERRAT_44   0
#define LV_FONT_MONTSERRAT_46   0
#define LV_FONT_MONTSERRAT_48   0

#define LV_FONT_MONTSERRAT_28_COMPRESSED 0
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW 0
#define LV_FONT_SIMSUN_14_CJK            0
#define LV_FONT_SIMSUN_16_CJK            0

/* Pixel perfect monospace fonts */
#define LV_FONT_UNSCII_8        0
#define LV_FONT_UNSCII_16       0

/* Custom font */
#define LV_FONT_CUSTOM_DECLARE

/** Default font */
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/** Enable FreeType library */
#define LV_USE_FREETYPE         0

/** Enable Tiny TTF library */
#define LV_USE_TINY_TTF         0

/** Enable vector fonts rendered by ThorVG */
#define LV_USE_VECTOR_FONT      0

/*=================
 *  TEXT SETTINGS
 *=================*/

#define LV_TXT_ENC LV_TXT_ENC_UTF8
#define LV_TXT_BREAK_CHARS " ,.;:-_)]}"
#define LV_TXT_LINE_BREAK_LONG_LEN 0
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN 3
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3
#define LV_USE_BIDI             0
#define LV_USE_ARABIC_PERSIAN_CHARS 0

/*==================
 * WIDGETS
 *================*/

#define LV_WIDGETS_HAS_DEFAULT_VALUE 1

#define LV_USE_ANIMIMG      1
#define LV_USE_ARC          1
#define LV_USE_BAR          1
#define LV_USE_BUTTON       1
#define LV_USE_BUTTONMATRIX 1
#define LV_USE_CALENDAR     1
#define LV_USE_CANVAS       1
#define LV_USE_CHART        1
#define LV_USE_CHECKBOX     1
#define LV_USE_DROPDOWN     1
#define LV_USE_IMAGE        1
#define LV_USE_IMAGEBUTTON  1
#define LV_USE_KEYBOARD     1
#define LV_USE_LABEL        1
#define LV_USE_LED          1
#define LV_USE_LINE         1
#define LV_USE_LIST         1
#define LV_USE_LOTTIE       0
#define LV_USE_MENU         1
#define LV_USE_MSGBOX       1
#define LV_USE_ROLLER       1
#define LV_USE_SCALE        1
#define LV_USE_SLIDER       1
#define LV_USE_SPAN         1
#define LV_USE_SPINBOX      1
#define LV_USE_SPINNER      1
#define LV_USE_SWITCH       1
#define LV_USE_TABLE        1
#define LV_USE_TABVIEW      1
#define LV_USE_TEXTAREA     1
#define LV_USE_TILEVIEW     1
#define LV_USE_WIN          1

/*==================
 * THEMES
 *==================*/

/** A simple theme */
#define LV_USE_THEME_SIMPLE     1

/** A theme designed for monochrome displays */
#define LV_USE_THEME_MONO       0

/*==================
 * LAYOUTS
 *==================*/

/** Flexbox layout */
#define LV_USE_FLEX     1

/** Grid layout */
#define LV_USE_GRID     1

/*==================
 * 3RD PARTY LIBS
 *==================*/

/* File system interfaces */
#define LV_USE_FS_STDIO         0
#define LV_USE_FS_POSIX         0
#define LV_USE_FS_WIN32         0
#define LV_USE_FS_FATFS         0
#define LV_USE_FS_MEMFS         0
#define LV_USE_FS_LITTLEFS      0
#define LV_USE_FS_ARDUINO_ESP_LITTLEFS 0
#define LV_USE_FS_ARDUINO_SD    0

/* Image decoders */
#define LV_USE_LODEPNG          0
#define LV_USE_LIBPNG           0
#define LV_USE_BMP              0
#define LV_USE_RLE              0
#define LV_USE_TJPGD            0
#define LV_USE_LIBJPEG_TURBO    0
#define LV_USE_GIF              0
#define LV_USE_BARCODE          0
#define LV_USE_QRCODE           0

/* FFmpeg */
#define LV_USE_FFMPEG           0

/* Others */
#define LV_USE_SNAPSHOT         0
#define LV_USE_MONKEY           0
#define LV_USE_GRIDNAV          0
#define LV_USE_FRAGMENT         0
#define LV_USE_IMGFONT          0
#define LV_USE_OBSERVER         1
#define LV_USE_IME_PINYIN       0
#define LV_USE_FILE_EXPLORER    0

/* Vector Graphics */
#define LV_USE_VECTOR_GRAPHIC   0

/* XML */
#define LV_USE_XML              0

/*==================
 * DISPLAY DRIVERS
 *==================*/

/** ST7796 LCD controller */
#define LV_USE_ST7796           1

/** Generic MIPI driver (required by ST7796) */
#define LV_USE_GENERIC_MIPI     1

/*==================
 * SYSMON (Performance Monitor)
 *==================*/

#define LV_USE_SYSMON           1
#if LV_USE_SYSMON
    /** Show CPU usage and FPS count */
    #define LV_USE_PERF_MONITOR 1
    #if LV_USE_PERF_MONITOR
        #define LV_USE_PERF_MONITOR_POS LV_ALIGN_BOTTOM_RIGHT
        #define LV_USE_PERF_MONITOR_LOG_MODE 0
    #endif

    /** Show used memory and memory fragmentation */
    #define LV_USE_MEM_MONITOR 0
#endif

/*==================
 * DEMOS
 *==================*/

#define LV_USE_DEMO_WIDGETS         1
#define LV_USE_DEMO_KEYPAD_AND_ENCODER 0
#define LV_USE_DEMO_BENCHMARK       1
#define LV_USE_DEMO_RENDER          0
#define LV_USE_DEMO_STRESS          0
#define LV_USE_DEMO_MUSIC           0
#define LV_USE_DEMO_VECTOR_GRAPHIC  0
#define LV_USE_DEMO_GLTF            0

#endif /* LV_CONF_H */
