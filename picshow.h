#ifndef PICSHOW_H
#define PICSHOW_H
#include <QWidget>
namespace Ui {
class PicShow;
}
class PicShow : public QWidget {
  Q_OBJECT
public:
  explicit PicShow(QWidget *parent = nullptr);
  ~PicShow() override;
  void ReloadPic();
public slots:
  void SlotSelectItem(const QString &path);
  void SlotUpdatePic(const QString &path);
  void SlotDeleteItem();
signals:
  void SigNextClicked();
  void SigPreClicked();

private:
  Ui::PicShow *ui;
  QString m_selectedPath;
  int m_generation = 0;
};
#endif
