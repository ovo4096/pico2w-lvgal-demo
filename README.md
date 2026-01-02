# Pico 2W LVGL Demo

一个在 Raspberry Pi Pico 2 W 上运行 LVGL 图形库的演示项目。使用 ST7796 显示屏和 FT6336U 电容触摸屏，经过深度优化以获得流畅的显示效果。

## ✨ 特性

- 🚀 针对 RP2350 深度优化，帧率高达 30+ FPS
- 🖥️ 支持 320×480 分辨率的 ST7796 显示屏
- 👆 集成 FT6336U 电容触摸，支持双点触控
- 🔄 DMA 异步传输，CPU 占用低
- 📦 开箱即用，配置简单

## 🛒 硬件清单

| 组件 | 规格 | 购买链接 |
|------|------|----------|
| 开发板 | Raspberry Pi Pico 2 W | - |
| 显示屏 | 3.5寸 ST7796 SPI TFT (320×480) | [淘宝](https://item.taobao.com/item.htm?id=758704987574) |
| 触摸屏 | FT6336U 电容触摸 (屏幕自带) | 同上 |

## 🔌 接线说明

将显示屏模块与 Pico 2 W 按如下方式连接：

**显示屏 (SPI)**
| 显示屏引脚 | Pico GPIO | 说明 |
|-----------|-----------|------|
| SCK | GP18 | SPI 时钟 |
| MOSI | GP19 | SPI 数据 |
| CS | GP17 | 片选 |
| DC | GP20 | 数据/命令 |
| RST | GP21 | 复位 |
| BL | GP22 | 背光 |

**触摸屏 (I2C)**
| 触摸屏引脚 | Pico GPIO | 说明 |
|-----------|-----------|------|
| SDA | GP2 | I2C 数据 |
| SCL | GP3 | I2C 时钟 |
| RST | GP4 | 复位 (可选) |
| INT | GP5 | 中断 (可选) |

> 💡 根据实际接线修改 `main.c` 中的引脚定义即可。

## 🚀 快速开始

### 环境要求

- [Pico SDK 2.2.0](https://github.com/raspberrypi/pico-sdk)
- CMake 3.13+
- Ninja 构建工具
- VS Code + Raspberry Pi Pico 扩展 (推荐)

### 编译

```bash
# 进入构建目录
cd build

# 配置项目
cmake .. -G Ninja

# 编译
ninja
```

使用 VS Code？直接运行 `Compile Project` 任务即可。

### 烧录

**方法一：拖拽烧录**
1. 按住 BOOTSEL 按钮连接 Pico
2. 将 `build/pico2w-lvgl-demo.uf2` 拖入弹出的 USB 驱动器

**方法二：调试器烧录**
运行 VS Code 的 `Flash` 任务（需要 CMSIS-DAP 调试器）

## ⚡ 性能优化详解

本项目在 RP2350 平台上实现了多项优化，以获得最佳显示性能：

### DMA 异步传输

像素数据通过 DMA 传输到 SPI，CPU 无需等待。当一帧数据在传输时，CPU 可以同时渲染下一帧。

```c
// DMA 传输完成后通过中断通知 LVGL
static void dma_irq_handler(void) {
    dma_channel_acknowledge_irq0(dma_channel);
    lv_display_flush_ready(current_disp);
}
```

### 双缓冲机制

采用半屏双缓冲策略（150KB × 2），在 DMA 传输一个缓冲区时，另一个缓冲区可同时进行渲染，大幅减少等待时间。

### 高速 SPI

SPI 时钟配置为最高速率，RP2350 会自动限制在硬件支持的最大速度（约 75MHz）。

### 快速字节交换

RGB565 大小端转换使用 32 位操作，一次处理两个像素：

```c
pixels32[i] = ((v & 0x00FF00FF) << 8) | ((v & 0xFF00FF00) >> 8);
```

### 精简配置

- 禁用未使用的 GPU 后端和颜色格式
- 关闭日志和调试功能
- 内存池设置为 128KB 以减少碎片

## ⚙️ 自定义配置

### 调整显示参数

在 `main.c` 中修改分辨率和 SPI 速度：

```c
#define DISP_HOR_RES    320
#define DISP_VER_RES    480
#define SPI_BAUDRATE    (1000 * 1000 * 1000)
```

### 校准触摸方向

如果触摸坐标与显示不匹配，调整这些参数：

```c
#define TOUCH_SWAP_XY       false  // 交换 X/Y 轴
#define TOUCH_INVERT_X      false  // 反转 X 轴
#define TOUCH_INVERT_Y      false  // 反转 Y 轴
```

### 修正颜色显示

颜色顺序不对（红蓝互换）？修改颜色标志：

```c
lv_st7796_create(..., LV_LCD_FLAG_BGR, ...);  // 或 LV_LCD_FLAG_NONE
```

颜色像负片一样反了？启用颜色反转：

```c
lv_st7796_set_invert(disp, true);
```

### 旋转屏幕

```c
lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);
```

可选值：`0`、`90`、`180`、`270`

## 📁 项目结构

```
pico2w-lvgl-demo/
├── main.c                  # 主程序：显示和触摸初始化
├── ft6336u.c/h             # FT6336U 触摸屏驱动
├── lv_conf.h               # LVGL 配置
├── CMakeLists.txt          # 构建配置
├── lvgl-9.4.0/             # LVGL 图形库
└── build/                  # 构建输出
```

## ❓ 常见问题

**Q: 编译报 Helium 汇编错误？**  
A: 项目已自动过滤不兼容的 Helium 文件，确保使用最新的 CMakeLists.txt。

**Q: 触摸屏没反应？**  
A: 检查 I2C 接线，确认地址为 0x38。可通过串口查看初始化日志。

**Q: 显示白屏？**  
A: 检查 SPI 接线，确认 RST 和 BL 引脚正确连接。

**Q: 帧率不够高？**  
A: 确认使用了双缓冲和 DMA 传输。可在 `lv_conf.h` 中调整 `LV_MEM_SIZE`。

## 📄 许可证

MIT License
