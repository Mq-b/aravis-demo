#include "CameraController.h"
#include <QDebug>
#include <cstring>

CameraController::CameraController(QObject *parent)
    : QObject(parent)
    , m_camera(nullptr)
    , m_stream(nullptr)
    , m_acquisitionTimer(new QTimer(this))
    , m_isConnected(false)
    , m_isAcquiring(false)
{
    connect(m_acquisitionTimer, &QTimer::timeout,
            this, &CameraController::onAcquisitionTimeout);
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

    // 如果未指定相机ID，连接第一个可用相机
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

    // 获取相机信息
    m_cameraModel = QString::fromUtf8(arv_camera_get_model_name(m_camera, nullptr));
    m_cameraVendor = QString::fromUtf8(arv_camera_get_vendor_name(m_camera, nullptr));
    m_cameraSerial = QString::fromUtf8(arv_camera_get_device_serial_number(m_camera, nullptr));

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
    arv_camera_set_exposure_time(m_camera, microseconds, &error);

    if (error) {
        QString errorMsg = QString::fromUtf8(error->message);
        g_error_free(error);
        emit errorOccurred(QString("设置曝光时间失败: %1").arg(errorMsg));
        return false;
    }

    emit parameterChanged("ExposureTime", microseconds);
    return true;
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

    GError *error = nullptr;
    arv_camera_set_gain(m_camera, gain, &error);

    if (error) {
        QString errorMsg = QString::fromUtf8(error->message);
        g_error_free(error);
        emit errorOccurred(QString("设置增益失败: %1").arg(errorMsg));
        return false;
    }

    emit parameterChanged("Gain", gain);
    return true;
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

    GError *error = nullptr;
    arv_camera_set_region(m_camera, x, y, width, height, &error);

    if (error) {
        QString errorMsg = QString::fromUtf8(error->message);
        g_error_free(error);
        emit errorOccurred(QString("设置ROI失败: %1").arg(errorMsg));
        return false;
    }

    emit parameterChanged("ROI", 0);  // 通知ROI已更改
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

    // 为流推送缓冲区
    for (int i = 0; i < 5; i++) {
        arv_stream_push_buffer(m_stream, arv_buffer_new(512 * 512, nullptr));
    }

    // 开始采集
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
    m_acquisitionTimer->start(33);  // 30 FPS
    emit acquisitionStarted();
    qDebug() << "图像采集已启动";

    return true;
}

void CameraController::stopAcquisition()
{
    if (!m_isAcquiring) {
        return;
    }

    m_acquisitionTimer->stop();

    GError *error = nullptr;
    arv_camera_stop_acquisition(m_camera, &error);
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

    // 如果正在连续采集，从流中获取
    if (m_isAcquiring && m_stream) {
        ArvBuffer *buffer = arv_stream_timeout_pop_buffer(m_stream, timeoutMs * 1000);
        if (buffer) {
            QImage image = convertArvBufferToQImage(buffer);
            arv_stream_push_buffer(m_stream, buffer);
            return image;
        }
        return QImage();
    }

    // 单帧采集模式
    GError *error = nullptr;
    ArvBuffer *buffer = arv_camera_acquisition(m_camera, timeoutMs * 1000, &error);

    if (!buffer || error) {
        QString errorMsg = error ? QString::fromUtf8(error->message) : "采集超时";
        if (error) g_error_free(error);
        emit errorOccurred(QString("单帧采集失败: %1").arg(errorMsg));
        return QImage();
    }

    QImage image = convertArvBufferToQImage(buffer);
    g_object_unref(buffer);

    return image;
}

// ========== 私有方法 ==========

void CameraController::onAcquisitionTimeout()
{
    if (!m_isAcquiring || !m_stream) {
        return;
    }

    // 非阻塞方式获取缓冲区
    ArvBuffer *buffer = arv_stream_try_pop_buffer(m_stream);
    if (buffer) {
        if (arv_buffer_get_status(buffer) == ARV_BUFFER_STATUS_SUCCESS) {
            QImage image = convertArvBufferToQImage(buffer);
            if (!image.isNull()) {
                emit newFrameAvailable(image);
            }
        }
        // 将缓冲区推回流
        arv_stream_push_buffer(m_stream, buffer);
    }
}

QImage CameraController::convertArvBufferToQImage(ArvBuffer *buffer)
{
    if (!buffer) {
        return QImage();
    }

    size_t bufferSize;
    const void *data = arv_buffer_get_data(buffer, &bufferSize);

    if (!data) {
        return QImage();
    }

    gint width, height;
    arv_buffer_get_image_region(buffer, nullptr, nullptr, &width, &height);

    ArvPixelFormat pixelFormat = arv_buffer_get_image_pixel_format(buffer);

    // 根据像素格式转换
    QImage image;

    switch (pixelFormat) {
        case ARV_PIXEL_FORMAT_MONO_8:
            // 8位灰度图
            image = QImage(static_cast<const uchar*>(data), width, height,
                          width, QImage::Format_Grayscale8).copy();
            break;

        case ARV_PIXEL_FORMAT_RGB_8_PACKED:
        case ARV_PIXEL_FORMAT_BGR_8_PACKED:
            // RGB/BGR 8位彩色图
            image = QImage(static_cast<const uchar*>(data), width, height,
                          width * 3, QImage::Format_RGB888).copy();
            if (pixelFormat == ARV_PIXEL_FORMAT_BGR_8_PACKED) {
                image = image.rgbSwapped();
            }
            break;

        default:
            qWarning() << "不支持的像素格式:" << pixelFormat;
            // 尝试作为灰度图处理
            image = QImage(static_cast<const uchar*>(data), width, height,
                          width, QImage::Format_Grayscale8).copy();
            break;
    }

    return image;
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
