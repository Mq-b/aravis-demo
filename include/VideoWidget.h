#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QWidget>
#include <QImage>
#include <QPainter>

class VideoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VideoWidget(QWidget *parent = nullptr);

    void setFrame(const QImage &frame);
    void clear();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QImage m_frame;
};

#endif
