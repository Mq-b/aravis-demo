#include "MainWindow.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QDateTime>
#include <QScrollArea>
#include <QSplitter>
#include <QStatusBar>
#include <QDebug>
#include <QElapsedTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_cameraController(new CameraController(this))
    , m_exposureMin(0), m_exposureMax(1000000)
    , m_gainMin(0), m_gainMax(24)
    , m_roiMaxWidth(1920), m_roiMaxHeight(1080)
{
    setupUI();

    // 连接信号槽
    connect(m_cameraController, &CameraController::cameraConnected,
            this, &MainWindow::onCameraConnected);
    connect(m_cameraController, &CameraController::cameraDisconnected,
            this, &MainWindow::onCameraDisconnected);
    connect(m_cameraController, &CameraController::newFrameAvailable,
            this, &MainWindow::onNewFrame);
    connect(m_cameraController, &CameraController::errorOccurred,
            this, &MainWindow::onError);
    connect(m_cameraController, &CameraController::acquisitionStarted,
            this, &MainWindow::onAcquisitionStarted);
    connect(m_cameraController, &CameraController::acquisitionStopped,
            this, &MainWindow::onAcquisitionStopped);
    connect(m_cameraController, &CameraController::fpsUpdated,
            this, &MainWindow::onFPSUpdated);

    updateUIState();
    logMessage("应用程序启动成功");
}

MainWindow::~MainWindow()
{
    if (m_cameraController->isConnected()) {
        m_cameraController->disconnectCamera();
    }
}

// ========== UI初始化 ==========

void MainWindow::setupUI()
{
    setWindowTitle("Aravis相机控制系统 v1.0");
    setMinimumSize(1200, 800);

    // 创建菜单栏
    createMenuBar();

    // 中央窗口
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    m_mainLayout = new QHBoxLayout(m_centralWidget);

    // 创建各个面板
    createCameraControlPanel();
    createImageDisplayPanel();

    // 使用分割器
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_controlPanel);
    splitter->addWidget(m_imageGroup);

    // 设置控制面板的最大宽度，让图像区域更大
    m_controlPanel->setMaximumWidth(400);

    // 设置初始分割比例：控制面板占1份，图像区域占5份
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 5);

    m_mainLayout->addWidget(splitter);

    // 状态栏
    m_statusBarLabel = new QLabel("就绪", this);
    statusBar()->addWidget(m_statusBarLabel);
}

void MainWindow::createMenuBar()
{
    QMenuBar *menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    // 文件菜单
    QMenu *fileMenu = menuBar->addMenu("文件(&F)");

    QAction *exitAction = new QAction("退出(&X)", this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
    fileMenu->addAction(exitAction);

    // 帮助菜单
    QMenu *helpMenu = menuBar->addMenu("帮助(&H)");

    QAction *aboutAction = new QAction("关于(&A)", this);
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QString aboutText = QString(
            "Aravis相机控制系统 v1.0\n\n"
            "基于Aravis库和Qt框架开发\n"
            "支持GenICam兼容的工业相机\n\n"
            "━━━━━━━━━━━━━━━━━━━━━━━━\n\n"
            "Aravis 版本: %1.%2.%3\n"
            "Qt 版本: %4\n"
            "编译器: %5\n\n"
            "━━━━━━━━━━━━━━━━━━━━━━━━"
        ).arg(arv_get_major_version())
         .arg(arv_get_minor_version())
         .arg(arv_get_micro_version())
         .arg(qVersion())
         .arg(
#ifdef __clang__
            QString("Clang %1.%2.%3").arg(__clang_major__).arg(__clang_minor__).arg(__clang_patchlevel__)
#elif defined(_MSC_VER)
            QString("MSVC %1").arg(_MSC_VER)
#elif defined(__GNUC__)
            QString("GCC %1.%2.%3").arg(__GNUC__).arg(__GNUC_MINOR__).arg(__GNUC_PATCHLEVEL__)
#else
            "Unknown"
#endif
         );

        QMessageBox::about(this, "关于", aboutText);
    });
    helpMenu->addAction(aboutAction);
}

void MainWindow::createCameraControlPanel()
{
    m_controlPanel = new QWidget(this);
    m_controlLayout = new QVBoxLayout(m_controlPanel);

    // 创建各个控制组
    createParameterPanel();
    createStatusPanel();

    // 相机连接组
    m_connectionGroup = new QGroupBox("相机连接", m_controlPanel);
    QVBoxLayout *connLayout = new QVBoxLayout(m_connectionGroup);

    m_cameraInfoLabel = new QLabel("未连接", m_connectionGroup);
    m_cameraInfoLabel->setWordWrap(true);
    m_cameraInfoLabel->setStyleSheet("QLabel { padding: 5px; background-color: #f0f0f0; }");

    m_connectButton = new QPushButton("连接相机", m_connectionGroup);
    m_disconnectButton = new QPushButton("断开连接", m_connectionGroup);

    connect(m_connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_disconnectButton, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);

    connLayout->addWidget(m_cameraInfoLabel);
    connLayout->addWidget(m_connectButton);
    connLayout->addWidget(m_disconnectButton);

    // 采集控制组
    m_acquisitionGroup = new QGroupBox("图像采集", m_controlPanel);
    QVBoxLayout *acqLayout = new QVBoxLayout(m_acquisitionGroup);

    m_startAcquisitionButton = new QPushButton("开始连续采集", m_acquisitionGroup);
    m_stopAcquisitionButton = new QPushButton("停止采集", m_acquisitionGroup);
    m_grabFrameButton = new QPushButton("单帧采集", m_acquisitionGroup);

    connect(m_startAcquisitionButton, &QPushButton::clicked,
            this, &MainWindow::onStartAcquisitionClicked);
    connect(m_stopAcquisitionButton, &QPushButton::clicked,
            this, &MainWindow::onStopAcquisitionClicked);
    connect(m_grabFrameButton, &QPushButton::clicked,
            this, &MainWindow::onGrabFrameClicked);

    acqLayout->addWidget(m_startAcquisitionButton);
    acqLayout->addWidget(m_stopAcquisitionButton);
    acqLayout->addWidget(m_grabFrameButton);

    // 添加到控制面板
    m_controlLayout->addWidget(m_connectionGroup);
    m_controlLayout->addWidget(m_acquisitionGroup);
    m_controlLayout->addWidget(m_parameterGroup);
    m_controlLayout->addWidget(m_statusGroup);
    m_controlLayout->addStretch();
}

void MainWindow::createParameterPanel()
{
    m_parameterGroup = new QGroupBox("参数设置", m_controlPanel);
    QVBoxLayout *paramLayout = new QVBoxLayout(m_parameterGroup);

    // === 曝光时间控制 ===
    QGroupBox *exposureGroup = new QGroupBox("曝光时间 (μs)", m_parameterGroup);
    QVBoxLayout *expLayout = new QVBoxLayout(exposureGroup);

    m_exposureSpinBox = new QDoubleSpinBox(exposureGroup);
    m_exposureSpinBox->setRange(1, 1000000);
    m_exposureSpinBox->setValue(10000);
    m_exposureSpinBox->setDecimals(0);
    m_exposureSpinBox->setSingleStep(1000);

    m_exposureSlider = new QSlider(Qt::Horizontal, exposureGroup);
    m_exposureSlider->setRange(1, 100000);
    m_exposureSlider->setValue(10000);

    connect(m_exposureSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onExposureChanged);
    connect(m_exposureSlider, &QSlider::valueChanged, this, [this](int value) {
        m_exposureSpinBox->blockSignals(true);
        m_exposureSpinBox->setValue(value);
        m_exposureSpinBox->blockSignals(false);
    });

    expLayout->addWidget(m_exposureSpinBox);
    expLayout->addWidget(m_exposureSlider);

    // === 增益控制 ===
    QGroupBox *gainGroup = new QGroupBox("增益 (dB)", m_parameterGroup);
    QVBoxLayout *gainLayout = new QVBoxLayout(gainGroup);

    m_gainSpinBox = new QDoubleSpinBox(gainGroup);
    m_gainSpinBox->setRange(0, 24);
    m_gainSpinBox->setValue(0);
    m_gainSpinBox->setDecimals(1);
    m_gainSpinBox->setSingleStep(0.5);

    m_gainSlider = new QSlider(Qt::Horizontal, gainGroup);
    m_gainSlider->setRange(0, 240);  // 0.1dB精度
    m_gainSlider->setValue(0);

    connect(m_gainSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onGainChanged);
    connect(m_gainSlider, &QSlider::valueChanged, this, [this](int value) {
        m_gainSpinBox->blockSignals(true);
        m_gainSpinBox->setValue(value / 10.0);
        m_gainSpinBox->blockSignals(false);
    });

    gainLayout->addWidget(m_gainSpinBox);
    gainLayout->addWidget(m_gainSlider);

    // === ROI控制 ===
    QGroupBox *roiGroup = new QGroupBox("ROI设置", m_parameterGroup);
    QGridLayout *roiLayout = new QGridLayout(roiGroup);

    roiLayout->addWidget(new QLabel("X:"), 0, 0);
    m_roiXSpinBox = new QSpinBox(roiGroup);
    m_roiXSpinBox->setRange(0, 4096);
    m_roiXSpinBox->setValue(0);
    roiLayout->addWidget(m_roiXSpinBox, 0, 1);

    roiLayout->addWidget(new QLabel("Y:"), 1, 0);
    m_roiYSpinBox = new QSpinBox(roiGroup);
    m_roiYSpinBox->setRange(0, 4096);
    m_roiYSpinBox->setValue(0);
    roiLayout->addWidget(m_roiYSpinBox, 1, 1);

    roiLayout->addWidget(new QLabel("宽度:"), 2, 0);
    m_roiWidthSpinBox = new QSpinBox(roiGroup);
    m_roiWidthSpinBox->setRange(64, 4096);
    m_roiWidthSpinBox->setValue(640);
    roiLayout->addWidget(m_roiWidthSpinBox, 2, 1);

    roiLayout->addWidget(new QLabel("高度:"), 3, 0);
    m_roiHeightSpinBox = new QSpinBox(roiGroup);
    m_roiHeightSpinBox->setRange(64, 4096);
    m_roiHeightSpinBox->setValue(480);
    roiLayout->addWidget(m_roiHeightSpinBox, 3, 1);

    m_setROIButton = new QPushButton("应用ROI", roiGroup);
    m_resetROIButton = new QPushButton("重置ROI", roiGroup);

    connect(m_setROIButton, &QPushButton::clicked, this, &MainWindow::onROIChanged);
    connect(m_resetROIButton, &QPushButton::clicked, this, [this]() {
        m_roiXSpinBox->setValue(0);
        m_roiYSpinBox->setValue(0);
        m_roiWidthSpinBox->setValue(m_roiMaxWidth);
        m_roiHeightSpinBox->setValue(m_roiMaxHeight);
        onROIChanged();
    });

    roiLayout->addWidget(m_setROIButton, 4, 0);
    roiLayout->addWidget(m_resetROIButton, 4, 1);

    // 添加所有子组到参数面板
    paramLayout->addWidget(exposureGroup);
    paramLayout->addWidget(gainGroup);
    paramLayout->addWidget(roiGroup);
}

void MainWindow::createImageDisplayPanel()
{
    m_imageGroup = new QGroupBox("图像预览", this);
    QVBoxLayout *imgLayout = new QVBoxLayout(m_imageGroup);

    // 图像显示标签（带滚动区域）
    QScrollArea *scrollArea = new QScrollArea(m_imageGroup);
    scrollArea->setWidgetResizable(false);
    scrollArea->setAlignment(Qt::AlignCenter);
    scrollArea->setMinimumSize(800, 600);  // 设置滚动区域最小尺寸

    m_imageLabel = new QLabel(scrollArea);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setMinimumSize(800, 600);  // 增加图像显示区域的最小尺寸
    m_imageLabel->setStyleSheet("QLabel { background-color: #2c2c2c; color: white; }");
    m_imageLabel->setText("无图像");

    scrollArea->setWidget(m_imageLabel);

    // 图像信息标签
    m_imageInfoLabel = new QLabel("分辨率: - | 格式: - | FPS: -", m_imageGroup);
    m_imageInfoLabel->setStyleSheet("QLabel { padding: 5px; background-color: #f0f0f0; }");

    imgLayout->addWidget(scrollArea, 1);
    imgLayout->addWidget(m_imageInfoLabel);
}

void MainWindow::createStatusPanel()
{
    m_statusGroup = new QGroupBox("状态日志", m_controlPanel);
    QVBoxLayout *statusLayout = new QVBoxLayout(m_statusGroup);

    m_statusTextEdit = new QTextEdit(m_statusGroup);
    m_statusTextEdit->setReadOnly(true);
    m_statusTextEdit->setMaximumHeight(150);
    m_statusTextEdit->setStyleSheet("QTextEdit { font-family: 'Consolas', monospace; font-size: 9pt; }");

    statusLayout->addWidget(m_statusTextEdit);
}

// ========== 槽函数实现 ==========

void MainWindow::onConnectClicked()
{
    logMessage("正在连接相机...");
    if (m_cameraController->connectCamera()) {
        updateParameterBounds();
        updateCameraInfo();
    }
}

void MainWindow::onDisconnectClicked()
{
    m_cameraController->disconnectCamera();
}

void MainWindow::onStartAcquisitionClicked()
{
    logMessage("启动连续采集...");
    m_cameraController->startAcquisition();
}

void MainWindow::onStopAcquisitionClicked()
{
    logMessage("停止采集...");
    m_cameraController->stopAcquisition();
}

void MainWindow::onGrabFrameClicked()
{
    logMessage("单帧采集...");
    QImage image = m_cameraController->grabSingleFrame();
    if (!image.isNull()) {
        onNewFrame(image);
        logMessage("单帧采集成功");
    }
}

void MainWindow::onExposureChanged(double value)
{
    if (m_cameraController->isConnected()) {
        m_exposureSlider->blockSignals(true);
        m_exposureSlider->setValue(static_cast<int>(value));
        m_exposureSlider->blockSignals(false);

        if (m_cameraController->setExposureTime(value)) {
            logMessage(QString("曝光时间设置为: %1 μs").arg(value, 0, 'f', 0));
        }
    }
}

void MainWindow::onGainChanged(double value)
{
    if (m_cameraController->isConnected()) {
        m_gainSlider->blockSignals(true);
        m_gainSlider->setValue(static_cast<int>(value * 10));
        m_gainSlider->blockSignals(false);

        if (m_cameraController->setGain(value)) {
            logMessage(QString("增益设置为: %1 dB").arg(value, 0, 'f', 1));
        }
    }
}

void MainWindow::onROIChanged()
{
    if (!m_cameraController->isConnected()) {
        return;
    }

    int x = m_roiXSpinBox->value();
    int y = m_roiYSpinBox->value();
    int width = m_roiWidthSpinBox->value();
    int height = m_roiHeightSpinBox->value();

    if (m_cameraController->setROI(x, y, width, height)) {
        logMessage(QString("ROI设置为: (%1, %2, %3x%4)")
                   .arg(x).arg(y).arg(width).arg(height));
    }
}

void MainWindow::onCameraConnected(const QString &model)
{
    logMessage(QString("相机已连接: %1").arg(model));
    m_statusBarLabel->setText(QString("已连接: %1").arg(model));
    updateUIState();
}

void MainWindow::onCameraDisconnected()
{
    logMessage("相机已断开");
    m_cameraInfoLabel->setText("未连接");
    m_statusBarLabel->setText("未连接");
    m_imageLabel->clear();
    m_imageLabel->setText("无图像");
    updateUIState();
}

void MainWindow::onNewFrame(const QImage &image)
{
    if (image.isNull()) {
        return;
    }

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    if (currentTime - m_lastFrameTime < UI_FRAME_INTERVAL) {
        return;
    }
    m_lastFrameTime = currentTime;

    QPixmap pixmap = QPixmap::fromImage(image);
    QSize labelSize = m_imageLabel->size();

    if (pixmap.width() > labelSize.width() || pixmap.height() > labelSize.height()) {
        pixmap = pixmap.scaled(labelSize, Qt::KeepAspectRatio, Qt::FastTransformation);
    }

    m_imageLabel->setPixmap(pixmap);
    m_imageLabel->resize(pixmap.size());
}

void MainWindow::onFPSUpdated(double fps)
{
    if (!m_cameraController->isAcquiring()) {
        return;
    }

    int x, y, width, height;
    if (!m_cameraController->getROI(x, y, width, height)) {
        return;
    }

    QString format = "灰度8位";
    m_imageInfoLabel->setText(QString("分辨率: %1x%2 | 格式: %3 | FPS: %4")
                              .arg(width)
                              .arg(height)
                              .arg(format)
                              .arg(fps, 0, 'f', 1));
}

void MainWindow::onError(const QString &errorMsg)
{
    logMessage(errorMsg, true);
    QMessageBox::warning(this, "错误", errorMsg);
}

void MainWindow::onAcquisitionStarted()
{
    logMessage("连续采集已启动");
    updateUIState();
}

void MainWindow::onAcquisitionStopped()
{
    logMessage("采集已停止");
    updateUIState();
}

// ========== 辅助函数 ==========

void MainWindow::updateCameraInfo()
{
    if (!m_cameraController->isConnected()) {
        return;
    }

    QString info = QString(
        "型号: %1\n"
        "厂商: %2\n"
        "序列号: %3"
    ).arg(m_cameraController->getCameraModel())
     .arg(m_cameraController->getCameraVendor())
     .arg(m_cameraController->getCameraSerialNumber());

    m_cameraInfoLabel->setText(info);
}

void MainWindow::updateParameterBounds()
{
    if (!m_cameraController->isConnected()) {
        return;
    }

    if (m_cameraController->getExposureTimeBounds(m_exposureMin, m_exposureMax)) {
        m_exposureSpinBox->setRange(m_exposureMin, m_exposureMax);
        m_exposureSlider->setRange(static_cast<int>(m_exposureMin),
                                   static_cast<int>(qMin(m_exposureMax, 100000.0)));

        double currentExposure = m_cameraController->getExposureTime();
        if (currentExposure > 0) {
            m_exposureSpinBox->blockSignals(true);
            m_exposureSpinBox->setValue(currentExposure);
            m_exposureSpinBox->blockSignals(false);
        }

        logMessage(QString("曝光范围: %1 - %2 μs")
                   .arg(m_exposureMin).arg(m_exposureMax));
    }

    if (m_cameraController->getGainBounds(m_gainMin, m_gainMax)) {
        m_gainSpinBox->setRange(m_gainMin, m_gainMax);
        m_gainSlider->setRange(static_cast<int>(m_gainMin * 10),
                              static_cast<int>(m_gainMax * 10));

        double currentGain = m_cameraController->getGain();
        if (currentGain >= 0) {
            m_gainSpinBox->blockSignals(true);
            m_gainSpinBox->setValue(currentGain);
            m_gainSpinBox->blockSignals(false);
        }

        logMessage(QString("增益范围: %1 - %2 dB")
                   .arg(m_gainMin).arg(m_gainMax));
    }

    if (m_cameraController->getROIBounds(m_roiMaxWidth, m_roiMaxHeight)) {
        m_roiXSpinBox->setMaximum(m_roiMaxWidth);
        m_roiYSpinBox->setMaximum(m_roiMaxHeight);
        m_roiWidthSpinBox->setMaximum(m_roiMaxWidth);
        m_roiHeightSpinBox->setMaximum(m_roiMaxHeight);

        int x, y, width, height;
        if (m_cameraController->getROI(x, y, width, height)) {
            m_roiXSpinBox->blockSignals(true);
            m_roiYSpinBox->blockSignals(true);
            m_roiWidthSpinBox->blockSignals(true);
            m_roiHeightSpinBox->blockSignals(true);

            m_roiXSpinBox->setValue(x);
            m_roiYSpinBox->setValue(y);
            m_roiWidthSpinBox->setValue(width);
            m_roiHeightSpinBox->setValue(height);

            m_roiXSpinBox->blockSignals(false);
            m_roiYSpinBox->blockSignals(false);
            m_roiWidthSpinBox->blockSignals(false);
            m_roiHeightSpinBox->blockSignals(false);
        }

        logMessage(QString("传感器尺寸: %1x%2")
                   .arg(m_roiMaxWidth).arg(m_roiMaxHeight));
    }
}

void MainWindow::updateUIState()
{
    bool isConnected = m_cameraController->isConnected();
    bool isAcquiring = m_cameraController->isAcquiring();

    // 连接按钮
    m_connectButton->setEnabled(!isConnected);
    m_disconnectButton->setEnabled(isConnected);

    // 采集按钮
    m_startAcquisitionButton->setEnabled(isConnected && !isAcquiring);
    m_stopAcquisitionButton->setEnabled(isConnected && isAcquiring);
    m_grabFrameButton->setEnabled(isConnected && !isAcquiring);

    // 参数控件
    m_parameterGroup->setEnabled(isConnected && !isAcquiring);
}

void MainWindow::logMessage(const QString &msg, bool isError)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString coloredMsg;

    if (isError) {
        coloredMsg = QString("<font color='red'>[%1] %2</font>").arg(timestamp, msg);
    } else {
        coloredMsg = QString("<font color='black'>[%1] %2</font>").arg(timestamp, msg);
    }

    m_statusTextEdit->append(coloredMsg);

    // 自动滚动到底部
    QTextCursor cursor = m_statusTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_statusTextEdit->setTextCursor(cursor);
}
