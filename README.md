# TFT_eSPI_test2

基于 Raspberry Pi Pico 2 W 和 LVGL 9.4.0 的 ST7796 TFT 显示屏驱动项目。

## 硬件配置

### 开发板
- Raspberry Pi Pico 2 W (RP2350)

### 显示屏
- ST7796 TFT LCD
- 分辨率: 320 x 480
- 颜色深度: RGB565 (16位)
- 接口: SPI

### 引脚连接

| 功能 | GPIO | 说明 |
|------|------|------|
| SPI SCK | 18 | SPI 时钟 |
| SPI MOSI | 19 | SPI 数据输出 |
| SPI MISO | 16 | SPI 数据输入 (未使用) |
| CS | 17 | 片选 |
| DC | 20 | 数据/命令选择 |
| RST | 21 | 复位 |
| BL | 22 | 背光控制 |

## 软件依赖

- Pico SDK 2.2.0
- LVGL 9.4.0

## 项目结构

```
TFT_eSPI_test2/
├── CMakeLists.txt          # CMake 构建配置
├── main.c                  # 主程序
├── lv_conf.h               # LVGL 配置文件
├── pico_sdk_import.cmake   # Pico SDK 导入
├── lvgl-9.4.0/             # LVGL 库
└── build/                  # 构建输出目录
```

## 编译和烧录

### 1. 配置项目
```bash
cd build
cmake .. -G Ninja
```

### 2. 编译
```bash
ninja
```

或使用 VS Code 任务: `Compile Project`

### 3. 烧录
使用 VS Code 任务: `Flash`

或将 `build/TFT_eSPI_test2.uf2` 拖拽到 Pico 的 USB 存储设备。

## 配置说明

### 显示参数

在 `main.c` 中修改:

```c
#define DISP_HOR_RES    320   // 水平分辨率
#define DISP_VER_RES    480   // 垂直分辨率
#define SPI_BAUDRATE    (40 * 1000 * 1000)  // SPI 速度 40MHz
```

### 颜色设置

颜色标志在 `lv_st7796_create()` 中设置:

```c
lv_display_t *disp = lv_st7796_create(
    DISP_HOR_RES, 
    DISP_VER_RES, 
    LV_LCD_FLAG_BGR,    // 颜色顺序
    lcd_send_cmd, 
    lcd_send_color
);
```

可用标志:
- `LV_LCD_FLAG_NONE` - RGB 顺序
- `LV_LCD_FLAG_BGR` - BGR 顺序 (红蓝交换)
- `LV_LCD_FLAG_MIRROR_X` - 水平镜像
- `LV_LCD_FLAG_MIRROR_Y` - 垂直镜像

可以使用 `|` 组合多个标志。

### 颜色反转

如果颜色显示为负片效果:

```c
lv_st7796_set_invert(disp, true);
```

### 屏幕旋转

```c
lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);  // 旋转 90°
```

## LVGL 配置

主要配置项在 `lv_conf.h`:

| 配置项 | 值 | 说明 |
|--------|-----|------|
| LV_COLOR_DEPTH | 16 | RGB565 颜色深度 |
| LV_MEM_SIZE | 32KB | LVGL 内存池大小 |
| LV_USE_ST7796 | 1 | 启用 ST7796 驱动 |
| LV_USE_GENERIC_MIPI | 1 | 启用通用 MIPI 驱动 |

## 常见问题

### 1. 颜色不正确 (红蓝互换)
将 `LV_LCD_FLAG_NONE` 改为 `LV_LCD_FLAG_BGR` 或反之。

### 2. 显示为负片
启用颜色反转: `lv_st7796_set_invert(disp, true);`

### 3. 显示方向不对
使用 `lv_display_set_rotation()` 或组合 `LV_LCD_FLAG_MIRROR_X/Y` 标志。

### 4. 编译时 Helium 汇编错误
项目已在 CMakeLists.txt 中过滤掉不兼容的 Helium 汇编文件。

## 许可证

MIT License
