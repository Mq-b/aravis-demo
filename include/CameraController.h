#ifndef CAMERACONTROLLER_H
#define CAMERACONTROLLER_H

#include <QObject>
#include <QImage>
#include <QString>
#include <QTimer>
#include <atomic>
#include <thread>

// 解决 Qt 和 GLib 的宏冲突
#ifdef signals
#undef signals
#endif

#include <arv.h>

// 恢复 Qt 的 signals 宏
#define signals Q_SIGNALS

/**
 * @brief 相机控制器类 - 封装Aravis相机操作
 *
 * 该类提供线程安全的相机控制接口，支持：
 * - 相机连接/断开
 * - 参数设置（曝光、增益、ROI等）
 * - 实时图像采集
 * - 错误处理
 */
class CameraController : public QObject
{
    Q_OBJECT

public:
    explicit CameraController(QObject *parent = nullptr);
    ~CameraController();

    // 相机连接相关
    bool connectCamera(const QString &cameraId = QString());
    void disconnectCamera();
    bool isConnected() const;

    // 获取相机信息
    QString getCameraModel() const;
    QString getCameraVendor() const;
    QString getCameraSerialNumber() const;

    // 参数控制
    bool setExposureTime(double microseconds);
    double getExposureTime() const;
    bool getExposureTimeBounds(double &min, double &max) const;

    bool setGain(double gain);
    double getGain() const;
    bool getGainBounds(double &min, double &max) const;

    // ROI控制
    bool setROI(int x, int y, int width, int height);
    bool getROI(int &x, int &y, int &width, int &height) const;
    bool getROIBounds(int &maxWidth, int &maxHeight) const;

    // 图像采集
    bool startAcquisition();
    void stopAcquisition();
    bool isAcquiring() const;

    // 单帧采集
    QImage grabSingleFrame(int timeoutMs = 5000);

Q_SIGNALS:
    void cameraConnected(const QString &model);
    void cameraDisconnected();
    void newFrameAvailable(const QImage &image);
    void errorOccurred(const QString &errorMsg);
    void acquisitionStarted();
    void acquisitionStopped();
    void parameterChanged(const QString &paramName, double value);
    void fpsUpdated(double fps);

private Q_SLOTS:
    void updateFPS();

private:
    void captureLoop();
    void cleanupResources();
    QString getLastGError() const;

    ArvCamera *m_camera;
    ArvStream *m_stream;
    std::atomic_bool m_running{false};
    std::thread m_captureThread;
    QTimer *m_fpsTimer;

    bool m_isConnected;
    bool m_isAcquiring;

    QString m_cameraModel;
    QString m_cameraVendor;
    QString m_cameraSerial;

    int m_frameCount;          // Qt层发送的帧数
    double m_currentFPS;
    double m_maxFPS = 120.0;

    // Aravis底层统计
    int m_arvReceivedCount;    // arv_stream_try_pop_buffer 获取到的总帧数
    int m_arvSuccessCount;     // 状态为SUCCESS的帧数
};

#endif // CAMERACONTROLLER_H
