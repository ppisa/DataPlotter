QT += core gui serialport widgets printsupport

CONFIG += c++11

SOURCES += \
    buffer.cpp \
    channel.cpp \
    main.cpp \
    mainwindow.cpp \
    myplot.cpp \
    myterminal.cpp \
    plotdata.cpp \
    qcustomplot.cpp \
    serialhandler.cpp \
    settings.cpp

HEADERS += \
    buffer.h \
    channel.h \
    enums.h \
    mainwindow.h \
    myplot.h \
    myterminal.h \
    plotdata.h \
    qcustomplot.h \
    serialhandler.h \
    settings.h

FORMS += mainwindow.ui

TRANSLATIONS += \
    translation_en.ts \
    translation_cz.ts

RESOURCES += resources.qrc
