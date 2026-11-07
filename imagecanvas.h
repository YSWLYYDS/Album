#pragma once
#include <QtWidgets>
class ImageCanvas : public QGraphicsView {
public:
  explicit ImageCanvas(QWidget *parent = nullptr) : QGraphicsView(parent) {
    setScene(new QGraphicsScene(this));
    setDragMode(ScrollHandDrag);
    setBackgroundBrush(QColor("#16233c"));
    setRenderHint(QPainter::SmoothPixmapTransform);
    setTransformationAnchor(AnchorUnderMouse);
  }
  void setImage(const QImage &image) {
    scene()->clear();
    scene()->addPixmap(QPixmap::fromImage(image));
    scene()->setSceneRect(scene()->itemsBoundingRect());
    fit();
  }
  void fit() {
    resetTransform();
    fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
    m_zoom = 1;
  }
  void zoom(double factor) {
    if (m_zoom * factor < 0.15 || m_zoom * factor > 20)
      return;
    scale(factor, factor);
    m_zoom *= factor;
  }

protected:
  void wheelEvent(QWheelEvent *event) override {
    zoom(event->angleDelta().y() > 0 ? 1.2 : 1 / 1.2);
    event->accept();
  }
  void mouseDoubleClickEvent(QMouseEvent *event) override {
    fit();
    event->accept();
  }

private:
  double m_zoom = 1;
};
