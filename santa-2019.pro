QT += core gui widgets charts
CONFIG += c++17

SOURCES += \
    costmodel.cpp \
    main.cpp \
    mainwindow.cpp \
    problemdata.cpp \
    solver.cpp

HEADERS += \
    costmodel.h \
    mainwindow.h \
    problemdata.h \
    solver.h

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
