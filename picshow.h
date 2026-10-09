#ifndef PICSHOW_H
#define PICSHOW_H

#include <QDialog>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

namespace Ui {
class PicShow;
}

class PicShow : public QDialog
{
    Q_OBJECT

public:
    explicit PicShow(QWidget *parent = nullptr);
    ~PicShow();
    void ReloadPic();
protected:
    bool event(QEvent *event) override;
private:
    void ShowPreNextBtns(bool b_show);
    Ui::PicShow *ui;
    QPropertyAnimation* _animation_show_pre;
    QPropertyAnimation* _animation_show_next;
    bool _b_btnvisible;
    QString _selected_path;
    QPixmap _pix_map;
public slots:
    void SlotSelectItem(const QString &path);
    void SlotUpdatePic(const QString& _path);
    void SlotDeleteItem();

signals:
    void SigNextClicked();
    void SigPreClicked();
};

#endif // PICSHOW_H
