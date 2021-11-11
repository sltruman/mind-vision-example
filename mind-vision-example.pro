QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    cameraview.cpp \
    exposuredialog.cpp \
    main.cpp \
    mainmenu.cpp \
    mainwindow.cpp \
    whitebalancedialog.cpp

HEADERS += \
    cameraview.h \
    exposuredialog.h \
    mainmenu.h \
    mainwindow.h \
    whitebalancedialog.h

FORMS += \
    cameraview.ui \
    devicetreewidgetitem.ui \
    exposuredialog.ui \
    mainmenu.ui \
    mainwindow.ui \
    whitebalancedialog.ui


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resource.qrc

DISTFILES += \
    theme/black.css

TRANSLATIONS += language/app_zh.ts \
                language/app_en.ts



