#include "thumbnailtask.h"
#include <QImageReader>

ThumbnailTask::ThumbnailTask(const QString &path, int size, int gen)
    : _path(path), _size(size), _gen(gen)
{

    setAutoDelete(false);//关掉自动删除，避免跨线程操作
}

void ThumbnailTask::run()
{

    QImageReader reader(_path);//按需解码器
    reader.setAutoTransform(true);//按图片里的 EXIF 方向标记自动旋转

    QSize orig = reader.size();
    if(orig.isValid() && orig.width() > 0 && orig.height() > 0)
    {
        reader.setScaledSize(orig.scaled(_size, _size, Qt::KeepAspectRatio));//预告解码器缩略图的目标尺寸
    }

    QImage img;
    if(!reader.read(&img))// 失败也要删，否则任务对象泄漏
    {
        deleteLater();
        return;
    }

    if(img.width() > _size * 2 || img.height() > _size * 2)//二次缩放
    {
        img = img.scaled(_size, _size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    emit sigThumbReady(_gen, _path, img);
    deleteLater();
}
