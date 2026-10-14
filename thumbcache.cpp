#include "thumbcache.h"

ThumbCache &ThumbCache::Instance()
{
    // 函数内 static：第一次调用时构造，程序退出时自动销毁（C++11 起初始化线程安全）
    static ThumbCache cache;
    return cache;
}

ThumbCache::ThumbCache()
    : _cap(200)
{

}

QPixmap ThumbCache::Get(const QString &key)
{
    auto iter = _map.find(key);
    if(iter == _map.end()){
        return QPixmap(); // 未命中：调用方自己去开子线程解码
    }
    // 命中：把该节点"摘下来接到队头"，splice 只移动链表指针、不复制 QPixmap 数据，O(1)
    _list.splice(_list.begin(), _list, iter.value());
    return iter.value()->second;
}

void ThumbCache::Put(const QString &key, const QPixmap &pix)
{
    auto iter = _map.find(key);
    if(iter != _map.end()){
        // 已存在：只更新图片内容并移到队头，不新增节点、不改变容量
        iter.value()->second = pix;
        _list.splice(_list.begin(), _list, iter.value());
        return;
    }
    // 新插入队头，map 记录位置
    _list.push_front({key, pix});
    _map[key] = _list.begin();
    // 超容量：淘汰队尾（最久未使用）。顺序必须"先删 map 再删 list 节点"，
    // 否则 list.pop_back() 之后 map 里就残留一个指向已销毁节点的悬空迭代器
    if(_map.size() > _cap){
        auto &tail = _list.back();
        _map.remove(tail.first);
        _list.pop_back();
    }
}
