#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QResizeEvent>
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    Ui::MainWindow *ui;
    QWidget* _protree;
    QWidget* _picshow;


protected:
    virtual void resizeEvent(QResizeEvent *event);
private slots:
    void SlotCreatePro(bool);// 槽：响应"创建项目"菜单
    void SlotOpenPro(bool);// 槽：响应"打开项目"菜单
signals:
    void SigOpenPro(const QString &path);// 信号：打开项目后把路径广播出去
};
#endif // MAINWINDOW_H
