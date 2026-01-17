# Aravis 相机控制系统

基于 Aravis 开源库和 Qt5 框架开发的 GenICam 工业相机控制软件。

## 项目特点

✅ **开源驱动** - 使用 Aravis 库，不依赖相机厂商 SDK
✅ **跨平台设计** - 支持 Windows/Linux（当前配置为 Windows）
✅ **模块化架构** - 控制逻辑与 UI 分离，易于扩展维护
✅ **实时预览** - 支持连续采集和单帧采集
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

## 故障排查

### 无法连接相机

1. **检查硬件连接**
   - 网口相机：确认网线已连接，相机指示灯正常
   - USB 相机：确认 USB 连接，驱动已安装

2. **检查网络设置**（GigE 相机）
   - 设置固定 IP，确保与相机在同一网段
   - 关闭防火墙或添加例外规则

3. **使用 Aravis 工具测试**

   ```bash
   arv-tool-0.10 --list-cameras
   ```

### 编译错误

1. **找不到 Aravis 库**
   - 确认 vcpkg 已正确安装 aravis
   - 检查 `VCPKG_LIB` 环境变量

2. **Qt MOC 错误**
   - 确保 `CMAKE_AUTOMOC` 已启用
   - 清理 build 目录重新编译

### 运行时 DLL 缺失

确保以下 DLL 在可执行文件目录或系统 PATH 中：

- `aravis-0.10.dll`
- `glib-2.0.dll`
- `gobject-2.0.dll`
- Qt5Core.dll, Qt5Gui.dll, Qt5Widgets.dll

## 技术参考

- **Aravis 官方文档**: <https://aravisproject.github.io/aravis/>
- **GenICam 标准**: <https://www.emva.org/standards-technology/genicam/>
- **Qt5 文档**: <https://doc.qt.io/qt-5/>

## 开发计划

### 短期计划（1-2周）

- 添加图像保存功能
- 实现参数配置文件
- 优化图像显示性能

### 中期计划（1个月）

- 支持多相机同时控制
- 添加触发模式
- 实现基础图像处理功能

### 长期计划

- 跨平台测试（Linux）
- 插件系统
- 自定义 GenICam 特性访问

## 许可证

本项目采用 MIT 许可证。

---

**版本**: v1.0
**更新日期**: 2026-01-17
