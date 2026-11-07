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
    void SetPixmap(QTreeWidgetItem* item);//外部设置初始图
    void Start();//开始播放
    void Stop();//停止播放
    void SlideNext();
    void SlidePre();
protected:
    virtual void paintEvent(QPaintEvent * event);
private:
    void UpSelectPixMap(QTreeWidgetItem* item);
    float _factor; // 0.0~1.0 的动画进度
    QTimer* _timer;// 动画心跳
    QPixmap _pixmap1;// 当前显示的图
    QPixmap _pixmap2;// 下一张图（淡入用）
    QTreeWidgetItem * _cur_item;
    QMap<QString,QTreeWidgetItem*> _map_items;
    bool _b_start;// 是否正在播放
public slots:
    void SlotUpSelectShow(QString path); // 预览条点了某张图 → 跳过去
    void SlotStartOrStop();// 播放按钮被点 → 切换播放/暂停
private slots:
    void TimeOut();
signals:
    void SignUpPreList(QTreeWidgetItem* ); // 通知预览条"有新图要加进来"
    void SigSelectItem(QTreeWidgetItem* );// 通知预览条"把这张高亮"
    void SigStart(); // 通知播放按钮"变成暂停图标"
    void SigStop();// 通知播放按钮"变成播放图标"
};

#endif // PICANIMATIONWID_H
