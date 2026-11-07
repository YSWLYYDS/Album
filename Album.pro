QT += widgets
QT += core gui
QT += sql
SOURCES += reviewrepository.cpp reviewworkspace.cpp
HEADERS += reviewrepository.h reviewworkspace.h reviewscanworker.h
HEADERS += imageprobe.h imagecanvas.h businessstyle.h fitimageview.h

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    confirmpage.cpp \
    importworker.cpp \
    main.cpp \
    mainwindow.cpp \
    picanimationwid.cpp \
    picbutton.cpp \
    picshow.cpp \
    picstatebtn.cpp \
    prelistitem.cpp \
    prelistwid.cpp \
    prosetpage.cpp \
    protree.cpp \
    protreeitem.cpp \
    protreewidget.cpp \
    removeprodialog.cpp \
    scanworker.cpp \
    slideshowdlg.cpp \
    thumbcache.cpp \
    thumbnailtask.cpp \
    wizard.cpp

HEADERS += \
    confirmpage.h \
    const.h \
    importworker.h \
    mainwindow.h \
    nodeinfo.h \
    picanimationwid.h \
    picbutton.h \
    picshow.h \
    picstatebtn.h \
    prelistitem.h \
    prelistwid.h \
    prosetpage.h \
    protree.h \
    protreeitem.h \
    protreewidget.h \
    removeprodialog.h \
    scanworker.h \
    slideshowdlg.h \
    thumbcache.h \
    thumbnailtask.h \
    wizard.h

FORMS += \
    confirmpage.ui \
    mainwindow.ui \
    picshow.ui \
    prosetpage.ui \
    protree.ui \
    removeprodialog.ui \
    slideshowdlg.ui \
    wizard.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    rc.qrc
    
# MinGW windres cannot read absolute icon paths containing Chinese characters.
# Keep the application icon in Qt resources instead of a native RC resource.
