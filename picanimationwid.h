#ifndef PICANIMATIONWID_H
#define PICANIMATIONWID_H
#include<QWidget>
#include <qtreewidget.h>
class PicAnimationWid : public QWidget
{
    Q_OBJECT
public:
    explicit PicAnimationWid(QWidget *parent = nullptr);
    ~PicAnimationWid();
    void SetPixmap(QTreeWidgetItem* item);
    void Start();
    void Stop();
    void SlideNext();
    void SlidePre();
protected:
    virtual void paintEvent(QPaintEvent * event);
private:
    void UpSelectPixMap(QTreeWidgetItem* item);
    float _factor;
    QTimer* _timer;
    QPixmap _pixmap1;
    QPixmap _pixmap2;
    QTreeWidgetItem * _cur_item;
    QMap<QString,QTreeWidgetItem*> _map_items;
    bool _b_start;
public slots:
    void SlotUpSelectShow(QString path);
    void SlotStartOrStop();
private slots:
    void TimeOut();
signals:
    void SignUpPreList(QTreeWidgetItem* );
    void SigSelectItem(QTreeWidgetItem* );
    void SigStart();
    void SigStop();
    void SigMusicPlay();//用户点播放/暂停按钮时控制背景音乐（切图不触发）
    void SigMusicPause();
};

#endif // PICANIMATIONWID_H
