TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .
LIBS += -lqjson

# Input
RESOURCES += stitchy.qrc

HEADERS += \
  canvas.h \
  cell.h \
  color.h \
  colormanager.h \
  common.h \
  document.h \
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
  colormanager.cpp \
  document.cpp \
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