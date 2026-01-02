# pico2w-lvgl-demo

基于 Raspberry Pi Pico 2 W 和 LVGL 9.4.0 的 ST7796 TFT 显示屏驱动项目，集成 FT6336U 电容触摸屏。

## 性能优化

本项目针对 RP2350 平台进行了多项 FPS 优化：

### 1. 高速 SPI 传输
```c
#define SPI_BAUDRATE    (1000 * 1000 * 1000)  // 请求 1GHz，实际约 75MHz
```
- RP2350 的 SPI 外设会自动限制到最大支持速率
- 相比默认 40MHz 提升约 87.5%

### 2. DMA 异步传输
- 使用 DMA 进行像素数据传输，CPU 无需等待 SPI 完成
- 在 DMA 传输期间 CPU 可执行其他渲染任务
- 通过中断通知 LVGL 刷新完成，实现真正的异步操作

### 3. 双缓冲机制
```c
#define DISP_BUF_LINES  240  // 半屏缓冲
static uint8_t disp_buf1[DISP_BUF_SIZE * 2];  // 150KB 缓冲区 1
static uint8_t disp_buf2[DISP_BUF_SIZE * 2];  // 150KB 缓冲区 2
```
- 半屏双缓冲 (320 × 240 × 2 = 150KB × 2)
- 一个缓冲区 DMA 传输时，另一个缓冲区可同时渲染
- 显著减少 CPU 等待时间

### 4. 优化的字节交换
```c
// 使用 32 位操作一次处理 2 个像素
uint32_t v = pixels32[i];
pixels32[i] = ((v & 0x00FF00FF) << 8) | ((v & 0xFF00FF00) >> 8);
```
- RGB565 大小端转换使用 32 位操作
- 一次处理 2 个像素，比逐字节交换快 2 倍

### 5. LVGL 配置优化 (lv_conf.h)
| 配置项 | 值 | 说明 |
|--------|-----|------|
| `LV_DEF_REFR_PERIOD` | 10ms | 100 FPS 刷新率上限 |
| `LV_MEM_SIZE` | 128KB | 足够大的内存池减少碎片 |
| `LV_DRAW_BUF_ALIGN` | 4 | 4字节对齐优化内存访问 |
| `LV_DRAW_SW_SUPPORT_RGB565` | 1 | 原生 RGB565 支持 |
| `LV_DRAW_SW_SHADOW_CACHE_SIZE` | 0 | 禁用阴影缓存节省内存 |

### 6. 禁用未使用功能
- 禁用所有 GPU 后端 (PXP, DMA2D, SDL 等)
- 禁用不需要的颜色格式支持
- 禁用日志和调试功能

## 硬件配置

### 开发板
- Raspberry Pi Pico 2 W (RP2350)

### 显示屏
- ST7796 TFT LCD
- 分辨率: 320 x 480
- 颜色深度: RGB565 (16位)
- 接口: SPI

### 触摸屏
- FT6336U 电容触摸控制器
- 接口: I2C (400kHz)
- 支持双点触控

### 引脚连接

#### 显示屏 (SPI)
| 功能 | GPIO | 说明 |
|------|------|------|
| SPI SCK | 18 | SPI 时钟 |
| SPI MOSI | 19 | SPI 数据输出 |
| SPI MISO | 16 | SPI 数据输入 (未使用) |
| CS | 17 | 片选 |
| DC | 20 | 数据/命令选择 |
| RST | 21 | 复位 |
| BL | 22 | 背光控制 |

#### 触摸屏 (I2C)
| 功能 | GPIO | 说明 |
|------|------|------|
| I2C SDA | 2 | I2C 数据 |
| I2C SCL | 3 | I2C 时钟 |
| RST | 4 | 复位 (可选) |
| INT | 5 | 中断 (可选) |

## 软件依赖

- Pico SDK 2.2.0
- LVGL 9.4.0

## 项目结构

```
pico2w-lvgl-demo/
├── CMakeLists.txt          # CMake 构建配置
├── main.c                  # 主程序
├── ft6336u.c               # FT6336U 触摸驱动
├── ft6336u.h               # FT6336U 头文件
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

或将 `build/pico2w-lvgl-demo.uf2` 拖拽到 Pico 的 USB 存储设备。

## 配置说明

### 显示参数

在 `main.c` 中修改:

```c
#define DISP_HOR_RES    320   // 水平分辨率
#define DISP_VER_RES    480   // 垂直分辨率
#define SPI_BAUDRATE    (1000 * 1000 * 1000)  // SPI 速度
```

### 触摸屏校准

如果触摸坐标与显示不匹配，调整以下参数:

```c
#define TOUCH_SWAP_XY       false  // 交换 X/Y 轴
#define TOUCH_INVERT_X      false  // 反转 X 轴
#define TOUCH_INVERT_Y      false  // 反转 Y 轴
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

## 常见问题

### 1. 颜色不正确 (红蓝互换)
将 `LV_LCD_FLAG_NONE` 改为 `LV_LCD_FLAG_BGR` 或反之。

### 2. 显示为负片
启用颜色反转: `lv_st7796_set_invert(disp, true);`

### 3. 显示方向不对
使用 `lv_display_set_rotation()` 或组合 `LV_LCD_FLAG_MIRROR_X/Y` 标志。

### 4. 编译时 Helium 汇编错误
项目已在 CMakeLists.txt 中过滤掉不兼容的 Helium 汇编文件。

### 5. 触摸屏无响应
- 检查 I2C 引脚接线
- 确认 I2C 地址为 0x38
- 查看串口输出确认初始化状态

## 许可证

MIT License
