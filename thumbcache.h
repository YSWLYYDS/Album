#ifndef THUMBCACHE_H
#define THUMBCACHE_H

#include <QHash>
#include <QPixmap>
#include <list>
#include <utility>

// 手写 LRU 缓存（Least Recently Used，最近最少使用淘汰策略）
// 结构：std::list 存数据（队头 = 最近使用，队尾 = 最久未用），
//       QHash 存 "key -> list迭代器"，实现 O(1) 定位。
// 之所以敢存迭代器：std::list 的迭代器在增删其他节点后依然有效，这是本设计的前提。
// 线程安全：缩略图结果通过信号回到主线程后才读写缓存，全程只在主线程访问，无需加锁。
class ThumbCache
{
public:
    // 全程序单例：轮播窗口每次打开都会新建 PreListWid，缓存必须活得比窗口久，
    // 否则"重开项目秒出图"无从谈起。函数内 static 写法是 C++11 保证的线程安全单例。
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
    int _cap;                                    // 缓存上限（张数），约 200 张 90x90 图 ≈ 6MB，可控
};

#endif // THUMBCACHE_H
