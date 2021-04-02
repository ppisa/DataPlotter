QT += core gui serialport widgets printsupport opengl

LIBS += -lOpenGL32

RC_ICONS = icons/icon.ico

DEFINES += QT_DEPRECATED_WARNINGS QCUSTOMPLOT_USE_OPENGL

CONFIG += c++11

SOURCES += \
    averager.cpp \
    clickablelabel.cpp \
    interpolator.cpp \
    main.cpp \
    mainwindow.cpp \
    mainwindow_autoset.cpp \
    mainwindow_cursors.cpp \
    mainwindow_cursors_xy.cpp \
    mainwindow_export.cpp \
    mainwindow_gui_preset.cpp \
    mainwindow_gui_slots.cpp \
    mainwindow_save_load.cpp \
    mainwindow_timed_events.cpp \
    myaxistickerwithunit.cpp \
    mycursorslider.cpp \
    mydial.cpp \
    mydoublespinboxwithunits.cpp \
    myelidedcombobox.cpp \
    myfftplot.cpp \
    mymainplot.cpp \
    mymodifiedqcptracer.cpp \
    myplot.cpp \
    mypow2spinbox.cpp \
    myscaledoublespinbox.cpp \
    myterminal.cpp \
    myxyplot.cpp \
    newserialparser.cpp \
    plotdata.cpp \
    plotmath.cpp \
    qcustomplot.cpp \
    serialreader.cpp \
    serialsettingsdialog.cpp \
    signalprocessing.cpp

HEADERS += \
    averager.h \
    clickablelabel.h \
    enumsDefinesConstants.h \
    interpolator.h \
    mainwindow.h \
    myaxistickerwithunit.h \
    mycursorslider.h \
    mydial.h \
    mydoublespinboxwithunits.h \
    myelidedcombobox.h \
    myfftplot.h \
    mymainplot.h \
    mymodifiedqcptracer.h \
    myplot.h \
    mypow2spinbox.h \
    myscaledoublespinbox.h \
    myterminal.h \
    myxyplot.h \
    newserialparser.h \
    plotdata.h \
    plotmath.h \
    qcustomplot.h \
    serialreader.h \
    serialsettingsdialog.h \
    signalprocessing.h

FORMS += mainwindow.ui \
    serialsettingsdialog.ui

TRANSLATIONS += \
    translations/translation_en.ts \
    translations/translation_cz.ts

RESOURCES += resources.qrc


