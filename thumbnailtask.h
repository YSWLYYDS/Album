#ifndef THUMBNAILTASK_H
#define THUMBNAILTASK_H

#include <QObject>
#include <QRunnable>
#include <QString>
#include <QImage>

// QObject + QRunnable 双继承：QRunnable 让它能被 QThreadPool 调度，
// QObject 让它能发信号把结果送回主线程。
// 陷阱：QObject 必须是第一基类，否则 moc 处理会出问题。
class ThumbnailTask : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit ThumbnailTask(const QString &path, int size, int gen);
    void run() override; // 在线程池的子线程里执行，绝不能在这里碰任何 QWidget/QPixmap

signals:
    // 参数用 QImage 而非 QPixmap：QImage 是纯像素数据可跨线程传递，
    // QPixmap 绑定主线程 GUI 子系统，子线程创建/传递都属未定义行为
    void sigThumbReady(int gen, const QString &path, const QImage &img);

private:
    QString _path; // 原图路径，也是 LRU 缓存的 key
    int _size;     // 目标缩略图边长（大边）
    int _gen;      // 提交任务那一刻的批次号快照；结果回来要和列表当前批次号比对，
                   // 对不上就丢弃——防止"清空列表后旧结果又冒出来"的竞态
};

#endif // THUMBNAILTASK_H
