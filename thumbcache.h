#ifndef THUMBCACHE_H
#define THUMBCACHE_H

#include <QHash>
#include <QPixmap>
#include <list>
#include <utility>

class ThumbCache
{
public:
    // 全程序单例：轮播窗口每次打开都会新建 PreListWid，缓存必须活得比窗口久，
    static ThumbCache &Instance();

    // 禁止拷贝/赋值：单例不该被复制出第二份
    ThumbCache(const ThumbCache&) = delete;
    ThumbCache& operator=(const ThumbCache&) = delete;

    QPixmap Get(const QString &key);            // 命中：节点移到队头并返回；未命中：返回空 QPixmap
    void Put(const QString &key, const QPixmap &pix); // 插入/更新到队头，超过容量淘汰队尾

private:
    ThumbCache();
    using ListIt = std::list<std::pair<QString,QPixmap>>::iterator;

    std::list<std::pair<QString,QPixmap>> _list; // 队头 = 最近使用
    QHash<QString, ListIt> _map;                 // key -> list 中的位置
    int _cap;                                    // 缓存上限
};

#endif // THUMBCACHE_H
