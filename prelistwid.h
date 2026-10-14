#ifndef PRELISTWID_H
#define PRELISTWID_H

#include <QListWidget>
#include <QTreeWidgetItem>
#include <QImage>

class PreListWid : public QListWidget
{
    Q_OBJECT
public:
    PreListWid(QWidget *parent=nullptr);
    virtual ~PreListWid();
private:
    QMap<QString,QListWidgetItem*> _set_items;
    void AddListItem(const QString &path);
    QPixmap MakeIconPixmap(const QPixmap &src) const; // 等比缩放 + 灰底居中合成 90x90 格子图
    void ClearList();                                 // 清空列表并递增批次号（保险丝，当前无调用点）
    int _global;
    QPoint _pos_origin;
    int _last_index;
    int _generation; // 批次号：清空列表时 +1，用来丢弃清空前提交的过期缩略图结果
public slots:
    void SlotUpPreList(QTreeWidgetItem* tree_item);
    void SlotUpSelect(QTreeWidgetItem *tree_item);
    void SlotItemPressed(QListWidgetItem * item);
    void SlotThumbReady(int gen, const QString &path, const QImage &img); // 子线程解码完成回到主线程
signals:
    void SigUpSelectShow(QString path);
};

#endif // PRELISTWID_H
