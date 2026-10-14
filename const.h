#ifndef CONST_H
#define CONST_H

#endif // CONST_H

enum TreeItemType{
    TreeItemPro=1,//表示项目的条目
    TreeItemDir=2,//表示项目的文件夹
    TreeItemPic=3,//表示项目的图片
};
//轮播按钮的6种状态
enum PicBtnState{
    PicBtnStateNormal=1,
    PicBtnStateHover=2,
    PicBtnStatePress=3,

    PicBtnState2Normal=4,
    PicBtnState2Hover=5,
    PicBtnState2Press=6,
};

const int PROGRESS_WIDTH=300;//进度条宽
const int PROGRESS_MAX=300;//进度条上限
const int PREITEM_SIZE=100;//预览格子大小
const int PREICON_SIZE=90;//格子内缩略图
