#include "CameraController.h"
#include <QDebug>
#include <QMetaObject>
#include <chrono>

CameraController::CameraController(QObject *parent)
    : QObject(parent)
    , m_camera(nullptr)
    , m_stream(nullptr)
    , m_fpsTimer(new QTimer(this))
    , m_isConnected(false)
    , m_isAcquiring(false)
    , m_frameCount(0)
    , m_currentFPS(0.0)
    , m_arvReceivedCount(0)
    , m_arvSuccessCount(0)
{
    connect(m_fpsTimer, &QTimer::timeout,
            this, &CameraController::updateFPS);
}

CameraController::~CameraController()
{
    cleanupResources();
}

bool CameraController::connectCamera(const QString &cameraId)
{
    if (m_isConnected) {
        emit errorOccurred("相机已连接，请先断开");
        return false;
    }

    GError *error = nullptr;

    if (cameraId.isEmpty()) {
        m_camera = arv_camera_new(nullptr, &error);
    } else {
        m_camera = arv_camera_new(cameraId.toUtf8().constData(), &error);
    }

    if (!m_camera || error) {
        QString errorMsg = error ? QString::fromUtf8(error->message) : "未找到相机";
        if (error) g_error_free(error);
        emit errorOccurred(QString("相机连接失败: %1").arg(errorMsg));
        return false;
    }

    m_cameraModel = QString::fromUtf8(arv_camera_get_model_name(m_camera, nullptr));
    m_cameraVendor = QString::fromUtf8(arv_camera_get_vendor_name(m_camera, nullptr));
    m_cameraSerial = QString::fromUtf8(arv_camera_get_device_serial_number(m_camera, nullptr));

    // 确保相机处于停止状态
    GError *stopError = nullptr;
    arv_camera_stop_acquisition(m_camera, &stopError);
    if (stopError) {
        // 如果相机本来就是停止状态，这个错误可以忽略
        qDebug() << "停止采集(如果有):" << stopError->message;
        g_error_free(stopError);
    }

    // 等待一下让相机稳定 确保第一次设置 ROI 成功
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    m_isConnected = true;
    emit cameraConnected(m_cameraModel);
    qDebug() << "相机连接成功:" << m_cameraModel << "SN:" << m_cameraSerial;

    return true;
}

void CameraController::disconnectCamera()
{
    if (!m_isConnected) {
        return;
    }

    stopAcquisition();
    cleanupResources();

    m_isConnected = false;
    emit cameraDisconnected();
    qDebug() << "相机已断开";
}

bool CameraController::isConnected() const
{
    return m_isConnected;
}

QString CameraController::getCameraModel() const
{
    return m_cameraModel;
}

QString CameraController::getCameraVendor() const
{
    return m_cameraVendor;
}

QString CameraController::getCameraSerialNumber() const
{
    return m_cameraSerial;
}

// ========== 曝光时间控制 ==========

bool CameraController::setExposureTime(double microseconds)
{
    if (!m_isConnected) {
        emit errorOccurred("相机未连接");
        return false;
    }

    GError *error = nullptr;

    // 必须先关闭自动曝光
    arv_camera_set_exposure_time_auto(m_camera, ARV_AUTO_OFF, &error);
    if (error) {
        QString errorMsg = QString::fromUtf8(error->message);
        g_error_free(error);
        emit errorOccurred(QString("关闭自动曝光失败: %1").arg(errorMsg));
        return false;
    }

    // Retry for USB3 Vision access-denied errors during initialization
    const int maxRetries = 3;
    for (int retry = 0; retry < maxRetries; ++retry) {
        arv_camera_set_exposure_time(m_camera, microseconds, &error);

        if (!error) {
            emit parameterChanged("ExposureTime", microseconds);
            return true;
        }

        if (error && g_error_matches(error, ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_TRANSFER_ERROR)) {
            g_error_free(error);
            error = nullptr;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        break;
    }

    if (error) {
        QString errorMsg = QString::fromUtf8(error->message);
        g_error_free(error);
        emit errorOccurred(QString("设置曝光时间失败: %1").arg(errorMsg));
        return false;
    }

    return false;
}

double CameraController::getExposureTime() const
{
    if (!m_isConnected) {
        return 0.0;
    }

    GError *error = nullptr;
    double exposure = arv_camera_get_exposure_time(m_camera, &error);

    if (error) {
        g_error_free(error);
        return 0.0;
    }

    return exposure;
}

bool CameraController::getExposureTimeBounds(double &min, double &max) const
{
    if (!m_isConnected) {
        return false;
    }

    GError *error = nullptr;
    arv_camera_get_exposure_time_bounds(m_camera, &min, &max, &error);

    if (error) {
        g_error_free(error);
        return false;
    }

    return true;
}

// ========== 增益控制 ==========

bool CameraController::setGain(double gain)
{
    if (!m_isConnected) {
        emit errorOccurred("相机未连接");
        return false;
    }
    
    // 先关闭自动增益
    GError *error = nullptr;
    arv_camera_set_gain_auto(m_camera, ARV_AUTO_OFF, &error);
    if (error) {
        QString errorMsg = QString::fromUtf8(error->message);
        g_error_free(error);
        emit errorOccurred(QString("关闭自动增益失败: %1").arg(errorMsg));
        return false;
    }

    const int maxRetries = 3;
    for (int retry = 0; retry < maxRetries; ++retry) {
        arv_camera_set_gain(m_camera, gain, &error);

        if (!error) {
            emit parameterChanged("Gain", gain);
            return true;
        }

        if (error && g_error_matches(error, ARV_DEVICE_ERROR, ARV_DEVICE_ERROR_TRANSFER_ERROR)) {
            g_error_free(error);
            error = nullptr;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        break;
    }

    if (error) {
        QString errorMsg = QString::fromUtf8(error->message);
        g_error_free(error);
        emit errorOccurred(QString("设置增益失败: %1").arg(errorMsg));
        return false;
    }

    return false;
}

double CameraController::getGain() const
{
    if (!m_isConnected) {
        return 0.0;
    }

    GError *error = nullptr;
    double gain = arv_camera_get_gain(m_camera, &error);

    if (error) {
        g_error_free(error);
        return 0.0;
    }

    return gain;
}

bool CameraController::getGainBounds(double &min, double &max) const
{
    if (!m_isConnected) {
        return false;
    }

    GError *error = nullptr;
    arv_camera_get_gain_bounds(m_camera, &min, &max, &error);

    if (error) {
        g_error_free(error);
        return false;
    }

    return true;
}

// ========== ROI控制 ==========

bool CameraController::setROI(int x, int y, int width, int height)
{
    if (!m_isConnected) {
        emit errorOccurred("相机未连接");
        return false;
    }

    if (m_isAcquiring) {
        emit errorOccurred("ROI设置失败: 采集正在运行，请先停止采集");
        return false;
    }

    GError *error = nullptr;

    qDebug() << "尝试设置ROI:" << x << y << width << height;
    qDebug() << "当前isAcquiring状态:" << m_isAcquiring;
    qDebug() << "当前stream指针:" << (void*)m_stream;

    arv_camera_set_region(m_camera, x, y, width, height, &error);

    if (error) {
        QString errorMsg = QString::fromUtf8(error->message);
        g_error_free(error);
        qDebug() << "setROI GError详情:" << errorMsg;
        emit errorOccurred(QString("设置ROI失败: %1").arg(errorMsg));
        return false;
    }

    qDebug() << "ROI设置成功";
    emit parameterChanged("ROI", 0);
    return true;
}

bool CameraController::getROI(int &x, int &y, int &width, int &height) const
{
    if (!m_isConnected) {
        return false;
    }

    GError *error = nullptr;
    arv_camera_get_region(m_camera, &x, &y, &width, &height, &error);

    if (error) {
        g_error_free(error);
        return false;
    }

    return true;
}

bool CameraController::getROIBounds(int &maxWidth, int &maxHeight) const
{
    if (!m_isConnected) {
        return false;
    }

    GError *error = nullptr;
    gint minWidth, minHeight;
    arv_camera_get_width_bounds(m_camera, &minWidth, &maxWidth, &error);

    if (error) {
        g_error_free(error);
        return false;
    }

    arv_camera_get_height_bounds(m_camera, &minHeight, &maxHeight, &error);

    if (error) {
        g_error_free(error);
        return false;
    }

    return true;
}

// ========== 图像采集 ==========

bool CameraController::startAcquisition()
{
    if (!m_isConnected) {
        emit errorOccurred("相机未连接");
        return false;
    }

    if (m_isAcquiring) {
        emit errorOccurred("采集已在运行");
        return false;
    }

    GError *error = nullptr;

    // 创建流 (需要5个参数: camera, callback, user_data, destroy, error)
    m_stream = arv_camera_create_stream(m_camera, nullptr, nullptr, nullptr, &error);
    if (!m_stream || error) {
        QString errorMsg = error ? QString::fromUtf8(error->message) : "创建流失败";
        if (error) g_error_free(error);
        emit errorOccurred(QString("启动采集失败: %1").arg(errorMsg));
        return false;
    }

    // Allocate buffers based on actual payload size
    gint payload_size = arv_camera_get_payload(m_camera, &error);
    if (error) {
        QString errorMsg = QString::fromUtf8(error->message);
        g_error_free(error);
        g_object_unref(m_stream);
        m_stream = nullptr;
        emit errorOccurred(QString("获取缓冲区大小失败: %1").arg(errorMsg));
        return false;
    }

    for (int i = 0; i < 50; i++) {
        arv_stream_push_buffer(m_stream, arv_buffer_new(payload_size, nullptr));
    }

    arv_camera_set_frame_rate(m_camera, 120.0, &error);
    if (error) {
        qDebug() << "设置帧率失败:" << error->message;
        g_error_free(error);
        error = nullptr;
    } else {
        double actual_fps = arv_camera_get_frame_rate(m_camera, nullptr);
        qDebug() << "相机帧率已设置为:" << actual_fps << "fps";
    }

    double exposure = arv_camera_get_exposure_time(m_camera, nullptr);
    qDebug() << "当前曝光时间:" << exposure << "μs";
    qDebug() << "理论最大帧率:" << (1000000.0 / exposure) << "fps";

    arv_camera_start_acquisition(m_camera, &error);
    if (error) {
        QString errorMsg = QString::fromUtf8(error->message);
        g_error_free(error);
        g_object_unref(m_stream);
        m_stream = nullptr;
        emit errorOccurred(QString("启动采集失败: %1").arg(errorMsg));
        return false;
    }

    m_isAcquiring = true;
    m_frameCount = 0;
    m_arvReceivedCount = 0;
    m_arvSuccessCount = 0;
    m_currentFPS = 0.0;
    m_running = true;

    m_captureThread = std::thread(&CameraController::captureLoop, this);
    m_fpsTimer->start(1000);
    emit acquisitionStarted();
    qDebug() << "图像采集已启动";

    return true;
}

void CameraController::stopAcquisition()
{
    if (!m_isAcquiring) {
        return;
    }

    m_running = false;

    if (m_captureThread.joinable()) {
        m_captureThread.join();
    }

    m_fpsTimer->stop();

    GError *error = nullptr;
    const int maxRetries = 3;

    for (int retry = 0; retry < maxRetries; ++retry) {
        arv_camera_stop_acquisition(m_camera, &error);

        if (!error) {
            break;
        }

        if (retry < maxRetries - 1) {
            g_error_free(error);
            error = nullptr;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }

    if (error) {
        qWarning() << "停止采集时出错:" << error->message;
        g_error_free(error);
    }

    if (m_stream) {
        g_object_unref(m_stream);
        m_stream = nullptr;
    }

    m_isAcquiring = false;
    emit acquisitionStopped();
    qDebug() << "图像采集已停止";
}

void CameraController::captureLoop()
{
    qDebug() << "采集线程启动";

    // 用于统计每秒的帧数
    auto lastLogTime = std::chrono::steady_clock::now();
    int loopCount = 0;              // 循环次数
    int localArvReceived = 0;       // 本秒内arv接收的帧数
    int localArvSuccess = 0;        // 本秒内成功的帧数
    int localQtSent = 0;            // 本秒内发送给Qt的帧数

    while (m_running) {
        ArvBuffer *buffer = arv_stream_try_pop_buffer(m_stream);
        loopCount++;

        if (buffer) {
            m_arvReceivedCount++;
            localArvReceived++;

            if (arv_buffer_get_status(buffer) == ARV_BUFFER_STATUS_SUCCESS) {
                m_arvSuccessCount++;
                localArvSuccess++;

                size_t buffer_size;
                const void *buffer_data = arv_buffer_get_data(buffer, &buffer_size);

                if (buffer_data) {
                    gint width, height;
                    arv_buffer_get_image_region(buffer, nullptr, nullptr, &width, &height);
                    ArvPixelFormat pixel_format = arv_buffer_get_image_pixel_format(buffer);

                    if (pixel_format == ARV_PIXEL_FORMAT_MONO_8 || pixel_format == 0x01080001) {
                        QImage image(static_cast<const uchar*>(buffer_data), width, height, width, QImage::Format_Grayscale8);
                        QImage imageCopy = image.copy();

                        QMetaObject::invokeMethod(this, [this, imageCopy = std::move(imageCopy)]() {
                            m_frameCount++;
                            emit newFrameAvailable(imageCopy);
                        }, Qt::QueuedConnection);

                        localQtSent++;
                    }
                }
            }
            arv_stream_push_buffer(m_stream, buffer);
        }

        // 每秒输出一次统计信息
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastLogTime).count();
        if (elapsed >= 1000) {
            qDebug() << "=== Aravis采集统计 ===";
            qDebug() << "循环次数/秒:" << loopCount;
            qDebug() << "Arv接收帧数/秒:" << localArvReceived << "(总计:" << m_arvReceivedCount << ")";
            qDebug() << "Arv成功帧数/秒:" << localArvSuccess << "(总计:" << m_arvSuccessCount << ")";
            qDebug() << "Qt发送帧数/秒:" << localQtSent;

            // 重置本秒计数器
            loopCount = 0;
            localArvReceived = 0;
            localArvSuccess = 0;
            localQtSent = 0;
            lastLogTime = now;
        }

        // std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    qDebug() << "采集线程停止";
}

bool CameraController::isAcquiring() const
{
    return m_isAcquiring;
}

QImage CameraController::grabSingleFrame(int timeoutMs)
{
    if (!m_isConnected) {
        emit errorOccurred("相机未连接");
        return QImage();
    }

    GError *error = nullptr;
    ArvBuffer *buffer = arv_camera_acquisition(m_camera, timeoutMs * 1000, &error);

    if (!buffer || error) {
        QString errorMsg = error ? QString::fromUtf8(error->message) : "采集超时";
        if (error) g_error_free(error);
        emit errorOccurred(QString("单帧采集失败: %1").arg(errorMsg));
        return QImage();
    }

    size_t buffer_size;
    const void *buffer_data = arv_buffer_get_data(buffer, &buffer_size);
    QImage image;

    if (buffer_data) {
        gint width, height;
        arv_buffer_get_image_region(buffer, nullptr, nullptr, &width, &height);
        ArvPixelFormat pixel_format = arv_buffer_get_image_pixel_format(buffer);

        if (pixel_format == ARV_PIXEL_FORMAT_MONO_8 || pixel_format == 0x01080001) {
            image = QImage(static_cast<const uchar*>(buffer_data), width, height, width, QImage::Format_Grayscale8).copy();
        }
    }

    g_object_unref(buffer);
    return image;
}

// ========== 私有方法 ==========

void CameraController::updateFPS()
{
    m_currentFPS = m_frameCount;
    qDebug() << ">>> Qt层FPS:" << m_currentFPS << "(Qt接收并显示的帧数)";
    m_frameCount = 0;
    emit fpsUpdated(m_currentFPS);
}

void CameraController::cleanupResources()
{
    if (m_isAcquiring) {
        stopAcquisition();
    }

    if (m_camera) {
        g_object_unref(m_camera);
        m_camera = nullptr;
    }

    m_cameraModel.clear();
    m_cameraVendor.clear();
    m_cameraSerial.clear();
}

QString CameraController::getLastGError() const
{
    // 辅助函数，用于获取最后的GError信息
    return QString("检查具体操作的错误信息");
}
