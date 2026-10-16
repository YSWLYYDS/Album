#ifndef THUMBNAILTASK_H
#define THUMBNAILTASK_H

#include <QObject>
#include <QRunnable>
#include <QString>
#include <QImage>

class ThumbnailTask : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit ThumbnailTask(const QString &path, int size, int gen);
    void run() override;

signals:

    void sigThumbReady(int gen, const QString &path, const QImage &img);//子线程全程用 QImage，回到主线程再转换

private:
    QString _path; // 原图路径，也是 LRU 缓存的 key
    int _size;     // 目标缩略图边长
    int _gen;
};

#endif // THUMBNAILTASK_H
