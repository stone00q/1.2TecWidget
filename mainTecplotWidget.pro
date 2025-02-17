QT       += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
#INCLUDEPATH += D:/VTK/VTK9.3-VS2015-OCCT7.7/VTK9.3-install/include/vtk-9.3
#LIBS += -L$$quote("D:/VTK/VTK9.3-VS2015-OCCT7.7/VTK9.3-install/lib/")
INCLUDEPATH += D:/VTK/VTK9.3.1-VS2019/debug/include/vtk-9.3/
LIBS += -L$$quote("D:/VTK/VTK9.3.1-VS2019/debug/lib/")
CONFIG(debug, debug|release) {
LIBS+=\
vtkCommonComputationalGeometry-9.3d.lib\
vtkCommonCore-9.3d.lib\
vtkCommonDataModel-9.3d.lib\
vtkCommonMath-9.3d.lib\
vtkCommonExecutionModel-9.3d.lib\
vtkFiltersCore-9.3d.lib\
vtkFiltersGeneral-9.3d.lib\
vtkFiltersExtraction-9.3d.lib\
vtkFiltersFlowPaths-9.3d.lib\
vtkFiltersGeneral-9.3d.lib\
vtkFiltersGeometry-9.3d.lib\
vtkFiltersSources-9.3d.lib\
vtkGUISupportQt-9.3d.lib\
vtkInteractionStyle-9.3d.lib\
vtkInteractionWidgets-9.3d.lib\
vtkIOCore-9.3d.lib\
vtkRenderingAnnotation-9.3d.lib\
vtkRenderingCore-9.3d.lib\
vtkRenderingFreeType-9.3d.lib\
vtkRenderingOpenGL2-9.3d.lib\
vtksys-9.3d.lib\
}else{
LIBS+=\
vtkCommonComputationalGeometry-9.3.lib\
vtkCommonCore-9.3.lib\
vtkCommonDataModel-9.3.lib\
vtkCommonMath-9.3.lib\
vtkCommonExecutionModel-9.3.lib\
vtkFiltersCore-9.3.lib\
vtkFiltersExtraction-9.3.lib\
vtkFiltersFlowPaths-9.3.lib\
vtkFiltersGeneral-9.3.lib\
vtkFiltersGeometry-9.3.lib\
vtkFiltersSources-9.3.lib\
vtkGUISupportQt-9.3.lib\
vtkInteractionStyle-9.3.lib\
vtkInteractionWidgets-9.3.lib\
vtkIOCore-9.3.lib\
vtkRenderingAnnotation-9.3.lib\
vtkRenderingCore-9.3.lib\
vtkRenderingFreeType-9.3.lib\
vtkRenderingOpenGL2-9.3.lib\
vtksys-9.3.lib\
}
SOURCES += \
    TecplotWidget.cpp \
    main.cpp \
    widget.cpp

HEADERS += \
    TecplotWidget.h \
    widget.h

FORMS += \
    widget.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
