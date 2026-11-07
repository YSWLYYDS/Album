#include "picshow.h"
#include "thumbnailtask.h"
#include "ui_picshow.h"
#include <QFileInfo>
#include <QShortcut>
#include <QThreadPool>
PicShow::PicShow(QWidget *parent) : QWidget(parent), ui(new Ui::PicShow) {
  ui->setupUi(this);
  setObjectName("legacyImageBrowser");
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  connect(ui->previousBtn, &QPushButton::clicked, this,
          &PicShow::SigPreClicked);
  connect(ui->nextBtn, &QPushButton::clicked, this, &PicShow::SigNextClicked);
  connect(ui->allowUpscale, &QCheckBox::toggled, ui->imageViewport,
          &FitImageView::setAllowUpscale);
  auto *previous = new QShortcut(QKeySequence(Qt::Key_Left), this);
  auto *next = new QShortcut(QKeySequence(Qt::Key_Right), this);
  previous->setContext(Qt::WidgetWithChildrenShortcut);
  next->setContext(Qt::WidgetWithChildrenShortcut);
  connect(previous, &QShortcut::activated, ui->previousBtn,
          &QPushButton::click);
  connect(next, &QShortcut::activated, ui->nextBtn, &QPushButton::click);
  SlotDeleteItem();
}
PicShow::~PicShow() { delete ui; }
void PicShow::ReloadPic() {
  // Resizing only repaints an already decoded image. It must not read the file
  // or change the size hints of the widgets managed by the layout.
  ui->imageViewport->update();
}
void PicShow::SlotSelectItem(const QString &path) {
  if (path.isEmpty()) {
    SlotDeleteItem();
    return;
  }
  const int generation = ++m_generation;
  m_selectedPath = path;
  ui->previousBtn->setEnabled(true);
  ui->nextBtn->setEnabled(true);
  const QString name = QFileInfo(path).fileName();
  ui->fileInfo->setText(name + " · 正在读取预览…");
  ui->fileInfo->setToolTip(path);
  ui->imageViewport->clear("正在读取图片…");
  auto *task = new ThumbnailTask(path, 4096, generation);
  connect(
      task, &ThumbnailTask::sigThumbReady, this,
      [this](int gen, const QString &readyPath, const QImage &image) {
        if (gen != m_generation || readyPath != m_selectedPath)
          return;
        const QString name = QFileInfo(readyPath).fileName();
        if (image.isNull()) {
          ui->imageViewport->clear(
              "无法读取图片，请确认源文件仍存在且格式有效");
          ui->fileInfo->setText(name + " · 读取失败");
          return;
        }
        ui->imageViewport->setImage(QPixmap::fromImage(image));
        ui->fileInfo->setText(QString("%1  ·  预览 %2 × %3 px  ·  等比适应窗口")
                                  .arg(name)
                                  .arg(image.width())
                                  .arg(image.height()));
      },
      Qt::QueuedConnection);
  QThreadPool::globalInstance()->start(task);
}
void PicShow::SlotUpdatePic(const QString &path) { SlotSelectItem(path); }
void PicShow::SlotDeleteItem() {
  ++m_generation;
  m_selectedPath.clear();
  ui->imageViewport->clear("从左侧目录中双击图片进行预览");
  ui->fileInfo->setText("图片预览会保持比例，默认不放大小图");
  ui->fileInfo->setToolTip({});
  ui->previousBtn->setEnabled(false);
  ui->nextBtn->setEnabled(false);
}
