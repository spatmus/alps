#-------------------------------------------------
#
# Project created by QtCreator 2018-08-07T11:43:33
#
#-------------------------------------------------

QT       += core gui widgets multimedia

TARGET = al-Qt
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    audiofileloader.cpp \
    settingsdialog.cpp \
    Speakers.cpp \
    Synchro.cpp \
    wavfile.cpp \
    signalview.cpp \
    sounddevice.cpp \
    mainloop.cpp \
    ../alglib/src/alglibinternal.cpp \
    ../alglib/src/alglibmisc.cpp \
    ../alglib/src/ap.cpp \
    ../alglib/src/dataanalysis.cpp \
    ../alglib/src/diffequations.cpp \
    ../alglib/src/fasttransforms.cpp \
    ../alglib/src/integration.cpp \
    ../alglib/src/interpolation.cpp \
    ../alglib/src/linalg.cpp \
    ../alglib/src/optimization.cpp \
    ../alglib/src/solvers.cpp \
    ../alglib/src/specialfunctions.cpp \
    ../alglib/src/statistics.cpp

HEADERS += \
        mainwindow.h \
    audiofileloader.h \
    settingsdialog.h \
    Speakers.hpp \
    Synchro.hpp \
    wavfile.h \
    signalview.h \
    sounddevice.h \
    mainloop.h \
    ../alglib/src/alglibinternal.h \
    ../alglib/src/alglibmisc.h \
    ../alglib/src/ap.h \
    ../alglib/src/dataanalysis.h \
    ../alglib/src/diffequations.h \
    ../alglib/src/fasttransforms.h \
    ../alglib/src/integration.h \
    ../alglib/src/interpolation.h \
    ../alglib/src/linalg.h \
    ../alglib/src/optimization.h \
    ../alglib/src/solvers.h \
    ../alglib/src/specialfunctions.h \
    ../alglib/src/statistics.h \
    ../alglib/src/stdafx.h

FORMS += \
        mainwindow.ui \
    settingsdialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resource.qrc

win32: RC_FILE = icon.rc

DISTFILES += \
    icon.ico \
    icon.rc

win32: LIBS += -lws2_32
