CONFIG += qt debug
TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .
LIBS += -lqjson

# Input
RESOURCES += stitchy.qrc

FORMS += \
  coloreditor.ui

HEADERS += \
  canvas.h \
  cell.h \
  color.h \
  coloreditor.h \
  colormanager.h \
  common.h \
  document.h \
  documetnio.h \
  editor.h \
  editoractions.h \
  globalstate.h \
  mainwindow.h \
  palettemodel.h \
  palettewidget.h \
  settings.h \
  sparsemap.h \
  stitch.h \
  utils.h

SOURCES += \
  canvas.cpp \
  cell.cpp \
  color.cpp \
  coloreditor.cpp \
  colormanager.cpp \
  document.cpp \
  documentio.cpp \
  editor.cpp \
  editoractions.cpp \
  globalstate.cpp \
  mainwindow.cpp \
  palettemodel.cpp \
  palettewidget.cpp \
  settings.cpp \
  sparsemap.cpp \
  stitch.cpp \
  utils.cpp \
  main.cpp
