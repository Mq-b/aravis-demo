#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include "CameraController.h"

/**
 * @brief 主窗口类 - 相机控制界面
 *
 * 提供完整的图形界面，包括：
 * - 相机连接控制
 * - 参数调节面板（曝光、增益、ROI）
 * - 实时图像预览
 * - 状态信息显示
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private Q_SLOTS:
    // 相机控制槽函数
    void onConnectClicked();
    void onDisconnectClicked();
    void onStartAcquisitionClicked();
    void onStopAcquisitionClicked();
    void onGrabFrameClicked();

    // 参数调节槽函数
    void onExposureChanged(double value);
    void onGainChanged(double value);
    void onROIChanged();

    // 相机控制器信号响应
    void onCameraConnected(const QString &model);
    void onCameraDisconnected();
    void onNewFrame(const QImage &image);
    void onError(const QString &errorMsg);
    void onAcquisitionStarted();
    void onAcquisitionStopped();
    void onFPSUpdated(double fps);

private:
    // UI初始化
    void setupUI();
    void createCameraControlPanel();
    void createParameterPanel();
    void createImageDisplayPanel();
    void createStatusPanel();
    void createMenuBar();

    // 辅助函数
    void updateCameraInfo();
    void updateParameterBounds();
    void updateUIState();
    void logMessage(const QString &msg, bool isError = false);

    // 相机控制器
    CameraController *m_cameraController;

    // === UI组件 ===

    // 中央布局
    QWidget *m_centralWidget;
    QHBoxLayout *m_mainLayout;

    // 左侧控制面板
    QWidget *m_controlPanel;
    QVBoxLayout *m_controlLayout;

    // 相机连接组
    QGroupBox *m_connectionGroup;
    QPushButton *m_connectButton;
    QPushButton *m_disconnectButton;
    QLabel *m_cameraInfoLabel;

    // 采集控制组
    QGroupBox *m_acquisitionGroup;
    QPushButton *m_startAcquisitionButton;
    QPushButton *m_stopAcquisitionButton;
    QPushButton *m_grabFrameButton;

    // 参数控制组
    QGroupBox *m_parameterGroup;

    // 曝光时间控制
    QLabel *m_exposureLabel;
    QDoubleSpinBox *m_exposureSpinBox;
    QSlider *m_exposureSlider;

    // 增益控制
    QLabel *m_gainLabel;
    QDoubleSpinBox *m_gainSpinBox;
    QSlider *m_gainSlider;

    // ROI控制
    QLabel *m_roiLabel;
    QSpinBox *m_roiXSpinBox;
    QSpinBox *m_roiYSpinBox;
    QSpinBox *m_roiWidthSpinBox;
    QSpinBox *m_roiHeightSpinBox;
    QPushButton *m_setROIButton;
    QPushButton *m_resetROIButton;

    // 图像显示区域
    QGroupBox *m_imageGroup;
    QLabel *m_imageLabel;
    QLabel *m_imageInfoLabel;

    // 状态信息区域
    QGroupBox *m_statusGroup;
    QTextEdit *m_statusTextEdit;

    // 状态栏标签
    QLabel *m_statusBarLabel;

    // 参数范围缓存
    double m_exposureMin, m_exposureMax;
    double m_gainMin, m_gainMax;
    int m_roiMaxWidth, m_roiMaxHeight;
};

#endif // MAINWINDOW_H
