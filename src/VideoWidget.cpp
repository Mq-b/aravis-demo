#include "VideoWidget.h"
#include <QPaintEvent>

VideoWidget::VideoWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(640, 480);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
    setStyleSheet("background-color: black;");
}

void VideoWidget::setFrame(const QImage &frame)
{
    if (frame.isNull()) {
        return;
    }

    m_frame = frame;
    update();
}

void VideoWidget::clear()
{
    m_frame = QImage();
    update();
}

void VideoWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

    if (m_frame.isNull()) {
        painter.fillRect(rect(), Qt::black);
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, "无图像");
        return;
    }

    QRect targetRect = rect();
    QSize imageSize = m_frame.size();

    if (imageSize.width() <= targetRect.width() && imageSize.height() <= targetRect.height()) {
        int x = (targetRect.width() - imageSize.width()) / 2;
        int y = (targetRect.height() - imageSize.height()) / 2;
        painter.drawImage(x, y, std::move(m_frame));
    } else {
        QSize scaledSize = imageSize.scaled(targetRect.size(), Qt::KeepAspectRatio);
        int x = (targetRect.width() - scaledSize.width()) / 2;
        int y = (targetRect.height() - scaledSize.height()) / 2;
        QRect drawRect(x, y, scaledSize.width(), scaledSize.height());
        painter.drawImage(drawRect, std::move(m_frame));
    }
}
