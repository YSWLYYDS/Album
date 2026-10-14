#include "thumbnailtask.h"
#include <QImageReader>

ThumbnailTask::ThumbnailTask(const QString &path, int size, int gen)
    : _path(path), _size(size), _gen(gen)
{
    // 关键一步：QThreadPool 默认在任务执行完后直接 delete 任务对象，
    // 但那发生在子线程；而 QObject 的删除必须发生在它的归属线程（这里创建于主线程），
    // 跨线程 delete 是未定义行为。所以关掉自动删除，改在 run() 末尾调 deleteLater()，
    // 删除动作会经由事件队列回到主线程执行，安全。
    setAutoDelete(false);
}

void ThumbnailTask::run()
{
    // QImageReader：按需解码。相比 QPixmap(path) 一次性把整张图解进内存，
    // 它支持"先读头部尺寸、再按目标尺寸解码"，大图不再吃几十 MB 内存
    QImageReader reader(_path);
    reader.setAutoTransform(true); // 按 EXIF 信息自动旋转（手机照片常见竖拍方向标记）

    // 只读图片文件头拿原始尺寸，不解码像素，几乎不耗时
    QSize orig = reader.size();
    if(orig.isValid() && orig.width() > 0 && orig.height() > 0){
        // 预告解码器：最终要缩到 _size 大小，解码阶段直接按小图采样输出。
        // 一张 8000x6000 的照片不必先解出 48MB 像素再缩小，这就是提速的本质
        reader.setScaledSize(orig.scaled(_size, _size, Qt::KeepAspectRatio));
    }

    QImage img;
    if(!reader.read(&img)){
        // 解码失败（文件损坏/格式不支持）：也得 deleteLater，否则任务对象泄漏
        deleteLater();
        return;
    }

    // 兜底：个别格式读不出头部尺寸（上面没设置 scaledSize），
    // 这种情况 read() 出来的是原图大小，这里再缩一次
    if(img.width() > _size * 2 || img.height() > _size * 2){
        img = img.scaled(_size, _size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    emit sigThumbReady(_gen, _path, img); // 跨线程信号，经事件队列回到主线程
    deleteLater(); // 在归属线程（主线程）的事件循环里被删除，安全
}
