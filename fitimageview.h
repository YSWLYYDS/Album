#pragma once
#include <QPainter>
#include <QPixmap>
#include <QWidget>

// The image is painted inside the viewport; its pixel dimensions never become
// a layout minimum, which prevents image/layout/resize feedback loops.
class FitImageView : public QWidget {
public:
  explicit FitImageView(QWidget *parent = nullptr) : QWidget(parent) {
    setObjectName("imageViewport");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(120, 100);
  }
  QSize sizeHint() const override { return {640, 420}; }
  QSize minimumSizeHint() const override { return {120, 100}; }
  void setImage(const QPixmap &image) {
    m_image = image;
    m_message.clear();
    update();
  }
  void clear(const QString &message) {
    m_image = {};
    m_message = message;
    update();
  }
  void setAllowUpscale(bool value) {
    m_upscale = value;
    update();
  }
  QSize imageSize() const { return m_image.size(); }
  QRectF imageRect() const {
    if (m_image.isNull())
      return {};
    const QRectF available = QRectF(contentsRect()).adjusted(16, 16, -16, -16);
    if (available.width() <= 0 || available.height() <= 0)
      return {};
    const QSizeF native(m_image.width() / devicePixelRatioF(),
                        m_image.height() / devicePixelRatioF());
    qreal scale = qMin(available.width() / native.width(),
                       available.height() / native.height());
    if (!m_upscale)
      scale = qMin(qreal(1), scale);
    const QSizeF size(native.width() * scale, native.height() * scale);
    return {available.center().x() - size.width() / 2,
            available.center().y() - size.height() / 2, size.width(),
            size.height()};
  }

protected:
  void paintEvent(QPaintEvent *) override {
    QPainter painter(this);
    painter.fillRect(rect(), QColor("#17253b"));
    if (m_image.isNull()) {
      painter.setPen(QColor("#9fb1cb"));
      painter.drawText(rect().adjusted(25, 20, -25, -20),
                       Qt::AlignCenter | Qt::TextWordWrap, m_message);
      return;
    }
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawPixmap(imageRect(), m_image, QRectF(m_image.rect()));
  }

private:
  QPixmap m_image;
  QString m_message = "从左侧目录中双击图片进行预览";
  bool m_upscale = false;
};
