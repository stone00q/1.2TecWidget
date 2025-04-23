QT       += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
#INCLUDEPATH += D:/VTK/VTK9.3-VS2015-OCCT7.7/VTK9.3-install/include/vtk-9.3
#LIBS += -L$$quote("D:/VTK/VTK9.3-VS2015-OCCT7.7/VTK9.3-install/lib/")
#INCLUDEPATH += D:/VTK/VTK9.3.1-VS2019/debug/include/vtk-9.3/
#LIBS += -L$$quote("D:/VTK/VTK9.3.1-VS2019/debug/lib/")
#INCLUDEPATH += E:/ParaView_VTK/paraview5.13.2-release/include/paraview-5.13/
#INCLUDEPATH += E:/ParaView_VTK/paraview5.13.2-release/include/paraview-5.13/AvtAlgorithms#怎么处理debug和release不同路径
#  #C:/visDev/VTK/VTK-Git/vtk-install/include/vtk-pv5.13/
##LIBS += -L$$quote("C:/visDev/VTK/VTK-Git/vtk-install/lib/")
##LIBS += -L$$quote("E:/ParaView_VTK/paraview5.13.2-release/lib/")

#CONFIG(debug, debug|release) {
#LIBS += -L$$quote("E:/ParaView_VTK/paraview5.13.2-de'bug/lib/")
#LIBS+=\
#vtkCommonComputationalGeometry-pv5.13d.lib\
#vtkCommonCore-pv5.13d.lib\
#vtkCommonDataModel-pv5.13d.lib\
#vtkCommonMath-pv5.13d.lib\
#vtkCommonExecutionModel-pv5.13d.lib\
#vtkFiltersCore-pv5.13d.lib\
#vtkFiltersGeneral-pv5.13d.lib\
#vtkFiltersExtraction-pv5.13d.lib\
#vtkFiltersFlowPaths-pv5.13d.lib\
#vtkFiltersGeneral-pv5.13d.lib\
#vtkFiltersGeometry-pv5.13d.lib\
#vtkFiltersSources-pv5.13d.lib\
#vtkGUISupportQt-pv5.13d.lib\
#vtkInteractionStyle-pv5.13d.lib\
#vtkInteractionWidgets-pv5.13d.lib\
#vtkIOCore-pv5.13d.lib\
#vtkRenderingAnnotation-pv5.13d.lib\
#vtkRenderingCore-pv5.13d.lib\
#vtkRenderingFreeType-pv5.13d.lib\
#vtkRenderingOpenGL2-pv5.13d.lib\
#vtksys-pv5.13d.lib\
#vtkCommonTransforms-pv5.13d.lib\
#vtkFiltersModeling-pv5.13d.lib\
#}else{
#LIBS += -L$$quote("E:/ParaView_VTK/paraview5.13.2-release/lib/")
#LIBS+=\
#vtkCommonComputationalGeometry-pv5.13.lib\
#vtkCommonCore-pv5.13.lib\
#vtkCommonDataModel-pv5.13.lib\
#vtkCommonMath-pv5.13.lib\
#vtkCommonExecutionModel-pv5.13.lib\
#vtkFiltersCore-pv5.13.lib\
#vtkFiltersExtraction-pv5.13.lib\
#vtkFiltersFlowPaths-pv5.13.lib\
#vtkFiltersGeneral-pv5.13.lib\
#vtkFiltersGeometry-pv5.13.lib\
#vtkFiltersSources-pv5.13.lib\
#vtkGUISupportQt-pv5.13.lib\
#vtkInteractionStyle-pv5.13.lib\
#vtkInteractionWidgets-pv5.13.lib\
#vtkIOCore-pv5.13.lib\
#vtkRenderingAnnotation-pv5.13.lib\
#vtkRenderingCore-pv5.13.lib\
#vtkRenderingFreeType-pv5.13.lib\
#vtkRenderingOpenGL2-pv5.13.lib\
#vtksys-pv5.13.lib\
#vtkCommonTransforms-pv5.13.lib\
#vtkFiltersModeling-pv5.13.lib\
#vtkIOXML-pv5.13.lib\#新加的读入相关
#visit_vtk-pv5.13.lib\
#vtkIOVisItBridge-pv5.13.lib\
#}
INCLUDEPATH += D:/VTK/VTK9.3.1-VS2019-OCCT7.7/VTK-9.3.1-install/include/vtk-9.3/
LIBS += -L$$quote("D:/VTK/VTK9.3.1-VS2019-OCCT7.7/VTK-9.3.1-install/lib/")
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
vtkCommonTransforms-9.3d.lib\
vtkFiltersModeling-9.3d.lib\
}else{
LIBS+=\
vtkCommonComputationalGeometry-9.3.lib\
vtkCommonCore-9.3.lib\
vtkCommonDataModel-9.3.lib\
vtkCommonMath-9.3.lib\
vtkCommonExecutionModel-9.3.lib\
vtkFiltersCore-9.3.lib\
vtkFiltersGeneral-9.3.lib\
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
vtkCommonTransforms-9.3.lib\
vtkFiltersModeling-9.3.lib\
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
