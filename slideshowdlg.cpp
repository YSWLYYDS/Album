#include "slideshowdlg.h"
#include "ui_slideshowdlg.h"
#include "prelistwid.h"
#include "protreewidget.h"
SlideShowDlg::SlideShowDlg(QWidget *parent, QTreeWidgetItem *frist_item, QTreeWidgetItem *last_item) :
    QDialog(parent),_first_item(frist_item),_last_item(last_item),
    ui(new Ui::SlideShowDlg)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Dialog|Qt::FramelessWindowHint);//无边框
    ui->slidepreBtn->SetIcons(":/icon/previous.png",":/icon/previous_hover.png",":/icon/previous_press.png");
    ui->slidenextBtn->SetIcons(":/icon/next.png",":/icon/next_hover.png",":/icon/next_press.png");
    ui->closeBtn->SetIcons(":/icon/closeshow.png",":/icon/closeshow_hover.png",":/icon/closeshow_press.png");
    ui->playBtn->SetIcons(":/icon/play.png",":/icon/play_hover.png",":/icon/play_press.png",":/icon/pause.png",":/icon/pause_hover.png",":/icon/pause_press.png");

    connect(ui->closeBtn,&QPushButton::clicked,this,&QDialog::reject);//reject 会发射 finished，便于外部停止音乐
    connect(ui->slidenextBtn,&QPushButton::clicked,this,&SlideShowDlg::SlotSlideNext);
    connect(ui->slidepreBtn,&QPushButton::clicked,this,&SlideShowDlg::SlotSlidePre);

    auto * prelistWid = dynamic_cast<PreListWid* >(ui->preListWidget);
    connect(ui->picAnimation,&PicAnimationWid::SignUpPreList,prelistWid,&PreListWid::SlotUpPreList);
    connect(ui->picAnimation,&PicAnimationWid::SigSelectItem,prelistWid,&PreListWid::SlotUpSelect);
    connect(prelistWid,&PreListWid::SigUpSelectShow,ui->picAnimation,&PicAnimationWid::SlotUpSelectShow);

    connect(ui->playBtn,&PicStateBtn::clicked,ui->picAnimation,&PicAnimationWid::SlotStartOrStop);
    connect(ui->picAnimation,&PicAnimationWid::SigStart,ui->playBtn,&PicStateBtn::SlotStart);
    connect(ui->picAnimation,&PicAnimationWid::SigStop,ui->playBtn,&PicStateBtn::SlotStop);

    //播放/暂停按钮同步控制背景音乐（parent 是 ProTreeWidget）
    auto * pro_tree=qobject_cast<ProTreeWidget*>(parent);
    if(pro_tree){
        connect(ui->picAnimation,&PicAnimationWid::SigMusicPlay,pro_tree,&ProTreeWidget::SlotStartMusic);
        connect(ui->picAnimation,&PicAnimationWid::SigMusicPause,pro_tree,&ProTreeWidget::SlotStopMusic);
    }

    ui->picAnimation->SetPixmap(_first_item);
    ui->picAnimation->Start();

}

SlideShowDlg::~SlideShowDlg()
{
    delete ui;
}

void SlideShowDlg::SlotSlideNext()
{
    ui->picAnimation->SlideNext();
}

void SlideShowDlg::SlotSlidePre()
{
    ui->picAnimation->SlidePre();
}
