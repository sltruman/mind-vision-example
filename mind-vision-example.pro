QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets charts

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    aboutdialog.cpp \
    cameraview.cpp \
    main.cpp \
    mainmenu.cpp \
    mainwindow.cpp \
    snapshotdialog.cpp \
    toplevelitemwidget.cpp

HEADERS += \
    aboutdialog.h \
    cameraview.h \
    mainmenu.h \
    mainwindow.h \
    snapshotdialog.h \
    toplevelitemwidget.h

FORMS += \
    aboutdialog.ui \
    cameraview.ui \
    mainmenu.ui \
    mainwindow.ui \
    snapshotdialog.ui \
    toplevelitemwidget.ui


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resource.qrc

DISTFILES += \
    theme/black.css \
    theme/black/leftside.css \
    theme/black/menubar.css \
    theme/black/preview.css \
    theme/black/rightside.css \
    theme/icon/1.png \
    theme/icon/10.png \
    theme/icon/12.png \
    theme/icon/2.png \
    theme/icon/3.png \
    theme/icon/4.png \
    theme/icon/5.png \
    theme/icon/6.png \
    theme/icon/8.png \
    theme/icon/9.png \
    theme/icon/LOGO.png \
    theme/icon/add.png \
    theme/icon/bottom-side.png \
    theme/icon/camera-background.png \
    theme/icon/camera.png \
    theme/icon/close.png \
    theme/icon/cn.png \
    theme/icon/collect.png \
    theme/icon/dir.png \
    theme/icon/down-list.png \
    theme/icon/download.png \
    theme/icon/expand.png \
    theme/icon/exposure.png \
    theme/icon/image.png \
    theme/icon/index.png \
    theme/icon/info.png \
    theme/icon/language-en.png \
    theme/icon/language-zh.png \
    theme/icon/laout.png \
    theme/icon/left-side.png \
    theme/icon/maximum.png \
    theme/icon/minimum.png \
    theme/icon/pause.png \
    theme/icon/play.png \
    theme/icon/playing.png \
    theme/icon/property.png \
    theme/icon/refresh.png \
    theme/icon/right-side.png \
    theme/icon/search.png \
    theme/icon/snapshot.png \
    theme/icon/stopped.png \
    theme/icon/top-side.png \
    theme/icon/trigger.png \
    theme/icon/upload.png \
    theme/icon/wb.png \
    theme/icon/zoom-in.png \
    theme/icon/zoom-out.png \
    theme/icon/下底.png \
    theme/icon/下拉.png \
    theme/icon/产品图.png \
    theme/icon/刷新.png \
    theme/icon/单个采集.png \
    theme/icon/右边底.png \
    theme/icon/图像处理.png \
    theme/icon/导入.png \
    theme/icon/导出.png \
    theme/icon/小箭头.png \
    theme/icon/展开.png \
    theme/icon/属性树.png \
    theme/icon/左边底.png \
    theme/icon/布局.png \
    theme/icon/常用属性.png \
    theme/icon/扩大.png \
    theme/icon/批量暂停.png \
    theme/icon/批量采集.png \
    theme/icon/抓拍.png \
    theme/icon/搜索.png \
    theme/icon/播放.png \
    theme/icon/文件夹1.png \
    theme/icon/暂停.png \
    theme/icon/曝光.png \
    theme/icon/最大化.png \
    theme/icon/白平衡.png \
    theme/icon/相机.png \
    theme/icon/相机拷贝2.png \
    theme/icon/箭头拷贝2.png \
    theme/icon/红色.png \
    theme/icon/绿色.png \
    theme/icon/缩小.png \
    theme/icon/缩进.png \
    theme/icon/触发控制.png \
    theme/icon/返回.png \
    theme/icon/链接.png \
    theme/icon/顶部.png

TRANSLATIONS += language/app_zh.ts \
                language/app_en.ts



