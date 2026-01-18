# Aravis 相机控制系统

基于 [Aravis](https://github.com/AravisProject/aravis) 开源库和 Qt5 框架开发的 Windows GenICam 工业相机控制软件。

> [!NOTE]
>
> **开发环境**：本项目基于 **Aravis 0.9.1** 与 **Qt 5.12+** 构建。
>
> **版本选型**：Aravis `0.9.1` 目前为 **Pre-release（预发布）** 版本。若追求更高稳定性，建议使用最新正式版 [`0.8.35`](https://www.google.com/search?q=[https://github.com/AravisProject/aravis/releases/tag/0.8.35](https://github.com/AravisProject/aravis/releases/tag/0.8.35))。
>
> **便捷集成**：由于 Aravis 依赖项较多，手动编译复杂度较高。推荐通过 `vcpkg` 包管理器一键安装正式版。
>
> **注意事项**：若切换至 `0.8.35` 版本，请务必同步修改 `CMakeLists.txt` 中的 `pkg_check_modules` 版本参数以确保匹配。

## 无需厂商驱动

**本项目在 Windows 平台使用通用 WinUSB 驱动，不安装相机厂商驱动，不依赖任何相机厂商 SDK 开发。**

工业相机能够跨品牌通用的核心是 **GenICam 标准**（由 EMVA 欧洲机器视觉协会维护）：

- **GenApi (XML 描述)**：每台相机内置 XML 文件，描述所有功能和寄存器地址
- **SFNC (标准命名)**：统一参数命名（如 `ExposureTime`、`Gain`）
- **USB3 Vision / GigE Vision**：物理层传输协议

**技术栈对比：**

| 传统方案 | 本项目方案 |
|---------|-----------|
| 厂商驱动（如海康MVS、巴斯勒Pylon） | **WinUSB** (Windows 通用驱动) |
| 厂商SDK封装 | **Aravis** (开源 GenICam 实现) |
| 单品牌绑定 | 支持所有符合 GenICam 标准的相机 |

**原理：** `Aravis` 直接解析相机内部的 `GenICam XML`，通过 `WinUSB` 读写寄存器和图像流，**完全绕过厂商 SDK**。

> [!IMPORTANT]
>
> [`GenICam`](https://www.emva.org/standards-technology/genicam/genicam-downloads/) 是一套全球标准，用来将工业相机与计算机软件应用（如机器视觉）衔接起来。它可实现图像处理、采集和传输中的措辞、接口和过程的同质化。
>
> 通过为所有用户提供一组通用名称和配置，无论供应商实现详情、功能或接口技术如何，都可确保通信。是 `GigE Vision`、`USB3 Vision`、`CoaXPress` 或 `Camera Link` 等高速视频标准的基础。

## 工业相机驱动配置

**USB3 Vision 相机（如 Blackfly S）：**

1. 插入相机后，Windows 设备管理器会识别为"USB3 Vision Device"
2. 使用 [Zadig](https://zadig.akeo.ie/) 工具安装 WinUSB 驱动：
   - 选择设备：`USB3 Vision Device`
   - 驱动选择：`WinUSB`
   - 点击 `Install Driver`

**GigE Vision 网口相机：**

- 无需额外驱动，直接通过以太网通信

## 项目特点

✅ **开源驱动** - 使用 Aravis 库，不依赖相机厂商 SDK

✅ **跨平台设计** - 支持 Windows/Linux（当前配置为 Windows）

✅ **模块化架构** - 控制逻辑与 UI 分离，易于扩展维护

✅ **高性能采集** - 独立线程采集，UI稳定60fps刷新

✅ **完整参数控制** - 曝光、增益、ROI 等参数可调

## 功能列表

### 已实现功能

- [x] 相机连接/断开
- [x] 相机信息显示（型号、厂商、序列号）
- [x] 曝光时间调节（微秒级）
- [x] 增益控制（dB）
- [x] ROI（感兴趣区域）设置
- [x] 连续图像采集（实时预览）
- [x] 单帧图像采集
- [x] 图像显示（支持灰度/彩色）
- [x] 状态日志输出
- [x] 参数范围自动检测

### 待扩展功能

- [ ] 图像保存（BMP/PNG/JPEG）
- [ ] 视频录制
- [ ] 白平衡调节
- [ ] 触发模式设置
- [ ] 多相机支持
- [ ] 图像处理（直方图、伪彩色等）
- [ ] 参数配置保存/加载

## 系统要求

### 开发环境

- **操作系统**: Windows 10/11 (64位)
- **编译器**: MSVC 2019+ 或 MinGW-w64
- **CMake**: 3.16 或更高版本
- **Qt**: 5.12 或更高版本
- **vcpkg**: 用于管理依赖库

### 运行环境

- Windows 10/11
- GigE Vision 或 USB3 Vision 兼容的工业相机（可选）
- Aravis 库及其依赖（GLib, GObject 等）

## 编译说明

### 1. 安装依赖

使用 vcpkg 安装 Aravis 和 Qt5:

```bash
# 安装 Aravis
vcpkg install aravis:x64-windows

# 安装 Qt5 (如果未安装)
vcpkg install qt5-base:x64-windows
```

### 2. 配置环境变量

设置 `VCPKG_LIB` 环境变量指向 vcpkg 安装目录:

```cmd
set VCPKG_LIB=C:\vcpkg\installed\x64-windows
```

### 3. 构建项目

```bash
# 创建构建目录
cd d:\project\aravis-demo
mkdir build
cd build

# 生成项目
cmake ..

# 编译
cmake --build . --config Release
```

### 4. 运行程序

```bash
cd build\Release\bin
.\aravis-demo.exe
```

## 项目结构

```txt
aravis-demo/
├── CMakeLists.txt           # CMake 配置文件
├── README.md                # 项目说明文档
├── include/                 # 头文件目录
│   ├── CameraController.h   # 相机控制核心类
│   └── MainWindow.h         # 主窗口界面类
├── src/                     # 源代码目录
│   ├── main.cpp             # 程序入口
│   ├── CameraController.cpp # 相机控制实现
│   └── MainWindow.cpp       # 主窗口实现
└── build/                   # 构建输出目录
```

## 代码架构

### CameraController 类

封装所有 Aravis 相机操作，提供线程安全的接口：

- **连接管理**: 相机连接、断开、状态查询
- **参数控制**: 曝光、增益、ROI 等参数设置
- **图像采集**: 连续采集、单帧采集
- **信号通知**: 通过 Qt 信号机制通知 UI

### MainWindow 类

主界面窗口，负责用户交互：

- **连接面板**: 相机连接控制和信息显示
- **参数面板**: 曝光/增益/ROI 可视化调节
- **图像显示**: 实时图像预览（支持缩放）
- **日志输出**: 操作记录和错误信息

## 使用说明

### 连接相机

1. 确保相机已正确连接（网口或 USB）
2. 点击"连接相机"按钮
3. 系统会自动连接第一个检测到的相机
4. 连接成功后显示相机型号和序列号

### 调节参数

**曝光时间**:

- 使用滑块或输入框调节（单位：微秒）
- 范围根据相机自动调整

**增益**:

- 使用滑块或输入框调节（单位：dB）
- 增益越大，图像越亮，但噪声也会增加

**ROI（感兴趣区域）**:

- 设置 X、Y 偏移和宽度、高度
- 点击"应用 ROI"生效
- 点击"重置 ROI"恢复全分辨率

### 图像采集

**连续采集**:

- 点击"开始连续采集"
- 图像会以约 30 FPS 刷新
- 点击"停止采集"结束

**单帧采集**:

- 确保未在连续采集模式
- 点击"单帧采集"获取一帧图像

## 许可证

本项目采用 MIT 许可证。
