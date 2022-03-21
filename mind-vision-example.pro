QT       += core gui xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets charts

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += C:\opencv\build\include \
               C:\Users\SLTru\Desktop\qtpropertybrowser\src

CONFIG(debug,debug|release) {
LIBS += -LC:\opencv\build\x64\vc15\lib -lopencv_world454d \
        -LC:\Users\SLTru\Desktop\qtpropertybrowser\lib -lQtSolutions_PropertyBrowser-headd
} else {
LIBS += -LC:\opencv\build\x64\vc15\lib -lopencv_world454 \
        -LC:\Users\SLTru\Desktop\qtpropertybrowser\lib -lQtSolutions_PropertyBrowser-head
}

SOURCES += \
    aboutdialog.cpp \
    calibrationdialog.cpp \
    cameraprocess.cpp \
    camerascene.cpp \
    cameraview.cpp \
    loadingdialog.cpp \
    main.cpp \
    mainmenu.cpp \
    mainwindow.cpp \
    offlinefpndialog.cpp \
    recorddialog.cpp \
    rightsidetitlebar.cpp \
    snapshotdialog.cpp \
    toplevelitemwidget.cpp

HEADERS += \
    GF120.h \
    aboutdialog.h \
    calibrationdialog.h \
    cameraprocess.h \
    camerascene.h \
    cameraview.h \
    loadingdialog.h \
    mainmenu.h \
    mainwindow.h \
    mainwindow_frameless.hpp \
    offlinefpndialog.h \
    recorddialog.h \
    rightsidetitlebar.h \
    snapshotdialog.h \
    toplevelitemwidget.h

FORMS += \
    aboutdialog.ui \
    calibrationdialog.ui \
    cameraview.ui \
    loadingdialog.ui \
    mainmenu.ui \
    mainwindow.ui \
    offlinefpndialog.ui \
    recorddialog.ui \
    rightsidetitlebar.ui \
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
    theme/black/calibration.css \
    theme/black/leftside.css \
    theme/black/menubar.css \
    theme/black/offlinefpn.css \
    theme/black/preview.css \
    theme/black/record.css \
    theme/black/rightside.css \
    theme/black/snapshot.css \
    theme/icon/1.png \
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

TRANSLATIONS += language/app_zh.ts

RC_ICONS = theme/icon/app.ico
QMAKE_LFLAGS += /MANIFESTUAC:\"level=\'requireAdministrator\' uiAccess=\'false\'\"
