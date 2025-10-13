#ifndef TECPLOTWIDGET_H
#define TECPLOTWIDGET_H

#include <QWidget>
#include <QMessageBox>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <vtkPoints.h>
#include <vtkFloatArray.h>
#include <vtkIdList.h>
#include <vtkCellType.h>

#include <vtkMultiBlockDataSet.h>
#include <vtkInformation.h>
#include <vtkUnstructuredGrid.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vtkSmartPointer.h>
#include <vtkDataSet.h>
#include <vtkDataSetMapper.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <QVTKOpenGLNativeWidget.h>

#include <vtkProperty.h>
#include <vtkLookupTable.h>
#include <vtkScalarBarActor.h>

#include <vtkImplicitPlaneWidget2.h>
#include <vtkImplicitPlaneRepresentation.h>
#include <vtkCutter.h>
#include <vtkPlane.h>
#include <vtkPolyDataMapper.h>
#include <QVector3D>
#include <QPoint>

#include <vtkContourFilter.h>
#include <vtkGeometryFilter.h>
#include <vtkArrayCalculator.h>
#include <vtkPointSource.h>
#include <vtkStreamTracer.h>
#include <vtkRungeKutta4.h>
#include <vtkPolyData.h>
#include <vtkPolyDataNormals.h>
#include <vtkMath.h>

#include <vtkArrowSource.h>
#include <vtkMaskPoints.h>
#include <vtkGlyph3D.h>

#include <vtkGradientFilter.h>
#include <vtkDoubleArray.h>

#include <vtkOrientationMarkerWidget.h>
#include <vtkAxesActor.h>
#include <cctype>
#include <vtkRemoveUnusedPoints.h>
class vtkProbeFilter;
class TecplotReader {
public:
    TecplotReader(){}
    ~TecplotReader(){}
    vtkMultiBlockDataSet* ReadTecplotData(const std::string& fileName);
private:
    // Helper functions:
    void pointsReader(int pointId, const std::string& line, int varNum, std::vector<vtkSmartPointer<vtkFloatArray>>& zoneData, vtkPoints* thePoints);
    void cellsReader(const std::string& cellType, const std::string& line, vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid);
    //vtkSmartPointer<vtkUnstructuredGrid> manualRemoveOverlap(vtkSmartPointer<vtkUnstructuredGrid> inputGrid, double tolerance = 1e-6);
};
class Contour
{
 // 对一个contour进行新建、setinputdata、加入contourlist、actor列表和renderer，全都默认命名方式
public:
    Contour(){}
    ~Contour(){}
    void Initialize(std::string contourName,std::string derivedName,std::map<std::string,vtkActor*>& actorsList,vtkRenderer* renderer);
    //std::string AddContour(std::string& contourDerived); // 添加一个contour,并返回默认命名
    double* SetActiveProperty(std::string propertyName); // 设置，返回这个属性的range
    int AddEntry(double value);  // 设置等值线/面抽取值，返回eid
    bool EditEntry(int entryId,double value);
    bool RemoveEntry(int entryId);   // 删除后entryid会前移
private:
    vtkDataSet* m_data;
    int m_valueNum = 0;//entry数量
    //bool propertyStatus = false;//是否指定过用于提取的属性参数
    std::string m_propertyName;
    vtkSmartPointer<vtkContourFilter> m_contourFilter;
    std::string m_contourName; // Contour 名称
    std::map<std::string, vtkActor*>* out_actorsList = nullptr; // 引用外部 actorsList
    vtkRenderer* out_renderer = nullptr; // 引用外部 renderer
    vtkSmartPointer<vtkActor> m_contourActor; // 延迟创建的 Actor
};
class Glyph{
    //对一个glyph进行新建，默认命名方式
public:
    Glyph(){}
    ~Glyph(){}
    void Initialize(std::string glyphName,std::string derivedName,std::map<std::string,vtkActor*>& actorsList,vtkRenderer* renderer);
    void SetGlyphVector(std::string vectorName);
    void SetGlyphSourceTipLength(double tipLength);  //箭头尖的长度
    void SetGlyphSourceTipRadius(double tipRadius);  //箭头尖的半径
    void SetGlyphSourceShaftRadius(double shaftRadius);  // 箭头杆的半径
    void SetGlyphSourceScaleFactor(double scaleFactor);  //factor大小
    void SetGlyphPointsNumber(int pointsNumber); //
private:
    bool InputState = false;//记录是否初始化过
    vtkDataSet* m_data;
    vtkSmartPointer<vtkArrowSource> m_arrowSource;
    vtkSmartPointer<vtkMaskPoints> m_maskPoints;
    vtkSmartPointer<vtkGlyph3D> m_glyphFilter;
};
class StreamTracer
{
public:
    StreamTracer(){}
    ~StreamTracer(){}
    void Initialize(std::string streamTracerName,std::string derivedName,std::map<std::string,vtkActor*>& actorsList,vtkRenderer* renderer);
private:
   vtkDataSet* m_data;
};

/**
 * @class TecplotWidget
 * @brief 读取并显示ASCII格式的tecplot数据
 * @details 能够读取的是ASCII格式的tecplot数据，能返回读取数据的各种信息，能够根据属性绘制云图、绘制等值线、
 */

class TecplotWidget : public QVTKOpenGLNativeWidget
{
    Q_OBJECT
public:
    explicit TecplotWidget(QWidget *parent = 0);
    ~TecplotWidget();

    /**
     * @brief SetFileName
     * @details 设置读取文件的路径，进行读取并显示。在读入数据时，如果没有"vel"或者"velocity"属性，则自动计算velocity速度属性
     * @param QString fileName，被读取的文件路径
     */
    void SetFileName(QString fileName);

    /**
     * @brief GetNumberOfBlock
     * @details 获取读入文件的block数目
     * @return int，返回读入文件的block数目
     */
    int GetNumberOfBlock();

    /****设置、获取渲染框背景颜色****/
    /**
     * @brief SetBackgroundColor
     * @param QColor color，默认是QColor(0,139,139)，希望设置的背景框颜色
     * @details 设置渲染窗口的颜色
     */
    void SetBackgroundColor(QColor color = QColor(0,139,139));

    /**
     * @brief GetBackgroundColor
     * @return QColor，返回渲染窗口现在的颜色
     */
    QColor GetBackgroundColor();


    /*****actor相关：获取actor列表、设置可见性、删除、获取属性表、设置固体颜色*****/
    /**
     * @brief GetActorList
     * @details 对于数据读入的多个block，对应actor命名和zoneName相同。但是对于截面、流线等操作产生的actor采用默认命名，一旦命名名字不可修改
     * @return QStringList，返回所有打开未关闭的actor的名字
     */
    QStringList GetActorList();

    /**
     * @brief ActorVisibilityOn
     * @param QString actorName，需要设置可见的actor名称
     * @return bool，如果actor名称不在atorlist里，会返回false，否则返回true
     * @details 将actor重新渲染显示。
     */
    bool ActorVisibilityOn(QString actorName);

    /**
     * @brief ActorVisibilityOff
     * @param QString actorName，需要设置不可见的actor名称
     * @return bool，如果actor名称不在atorlist里，会返回false，否则返回true
     * @details 将actor设置为不可见。
     */
    bool ActorVisibilityOff(QString actorName);

    /**
     * @brief RemoveActor
     * @param QString actorName，需要删除的actor名称
     * @return bool，如果actor名称不在atorlist里，会返回false，否则返回true
     * @details 删除指定actor
     */
    bool RemoveActor(QString actorName);

    /**
     * @brief GetNumberOfProperty
     * @details 获取数据的属性数量。
     * 注意如果读入数据本身没有三维速度，在最初读入数据时会自动通过“u*iHat + v*jHat + w*kHat”计算速度velocity并加入数据属性中
     * @param QString actorName，需要进行属性数目查找的actor名字，可通过GetActorList()函数获取所有的actor名称
     * @return int，返回数据的属性数量
     */
    int GetNumberOfProperty(QString actorName);

    /**
     * @brief GetPropertyList
     * @details 获取数据属性列表。
     * 注意如果读入数据本身没有三维速度，在最初读入数据时会自动通过“u*iHat + v*jHat + w*kHat”计算速度velocity并加入数据属性中
     * @param QString actorName，需要进行属性数目查找的actor名字，可通过GetActorList()函数获取所有的actor名称
     * @return QStringList，返回所有属性的名字
     */
    QStringList GetPropertyList(QString actorName);

    /**
     * @brief GetPropertyName
     * @param QString actorName，需要查询属性的actor名称，可通过GetActorList()函数获取所有的actor名称
     * @param int id，需要查询的属性的id。
     * id从0开始
     * @return QString，返回第i个属性的名称
     */
    QString GetPropertyName(QString actorName, int id);

    /**
     * @brief GetPropertyBounds
     * @param QString actorName，进行查询的actor名称
     * @param QString propertyName，进行查询的属性名称
     * @return vector<double>,返回一个包含最小值和最大值的 double数组。如果查询到的actorname或propertyName不合法，则vector为空。
     * 如果查询到的是向量，则是size=6，bounds[0]、bounds[1]表示第一个分量的bounds。
     */
    std::vector<double> GetPropertyBounds(QString actorName, QString propertyName);

    /****设置Actor颜色、颜色映射****/
    /**
     * @brief SetSolidColor
     * @param QString actorName，需要进行颜色设置的actor名称
     * @param QColor color，颜色参数。默认浅灰色QColor(211,211,211)
     * @details 对某个actor的固体颜色进行设置,如果之前执行过颜色映射，会关闭颜色映射
     */
    void SetSolidColor(QString actorName,QColor color=QColor(211,211,211));
    /**
     * @brief SetOpacity
     * @param QString actorName，需要进行透明度设置的actor名称（会关闭到颜色映射）
     * @param double opacity，透明度设置，0为完全透明，1为完全透明
     */
    void SetSolidOpacity(QString actorName, double opacity);
    /**
     * @brief GetSolidColor
     * @param QString actorName,需要获取固体颜色的actor名称
     * @return QColor，返回查询的actor的固体颜色。
     * 注意如果该actor打开了颜色映射，仍然会返回默认的颜色值（如果没有被明确设置为单一颜色），但这并不反映实际渲染的颜色。
     */
    QColor GetSolidColor(QString actorName);

    /**
     * @brief SetColorMapOn
     * @param QString propertyName，进行颜色映射的标量属性。
     * @details 对指定的actor指定属性进行颜色映射。需要自行验证属性变量名是否合法。
     */
    void SetColorMapOn(QString propertyName);

    /**
     * @brief SetColorMapOff
     * @details关闭所有颜色映射
     */
    void SetColorMapOff();
    /***颜色映射：指定色阶数量***/
    /**
     * @brief SetNumberOfColor
     * @param QString actorName，需要设置色阶数量的actor名称
     * @param int colorNum，色阶数量
     * @return bool，如果改模型没有打开过颜色映射则return false，否则return ture
     */
    bool SetNumberOfColor(int colorNum);
    /***颜色映射：指定色阶边界****/
    /**
     * @brief SetColorMapBounds
     * @param QString actorName，需要设置颜色映射bounds的actor名称
     * @param double low，下界
     * @param double high，上界
     * @return bool，如果该模型没有打开过颜色映射则return false，否则return ture（默认是范围内的
     */
    bool SetColorMapBounds(double low,double high);

    /**开启line表达**/
    /**
     * @brief SetColorLineOn
     * @details根据颜色映射的色阶数量决定等值线条数
     */
    void SetColorLineOn();
    /**
     * @brief SetColorLineOff
     * @details 关闭根据色阶生成等值线
     */
    void SetColorLineOff();

    /****设置cut****/
    /**
     * @brief AddSliceWidget添加截面的交互器
     * @param QString derivedActorName，交互器的输入actor
     * @return 返回这个交互器进行截面的名字，默认命名Slice1...
     * @details 针对指定actor添加一个截面的交互器，交互器名称和使用该交互器截面操作产生的actor相同。采用默认命名方式“Slice”+第“i”个截面
     */
    QString AddSliceWidget(QString derivedActorName);
    /**
     * @brief 激活指定切片的交互器，允许用户通过控件交互
     * @param sliceName 切片名称（由AddSliceWidget返回的名称）
     * @return 如果找到对应切片且操作成功返回true，否则false
     */
    bool EnableSliceInteraction(QString sliceName);
    /**
     * @brief 隐藏指定名称的切片交互器（保留平面，但禁用控件显示）
     * @param sliceName 交互器名称（由 AddSliceWidget 返回的名称）
     * @return 如果交互器存在且操作成功返回 true，否则返回 false
     */
    bool HideSliceWidget(QString sliceName);
    /**
     * @brief 显示指定名称的切片交互器（恢复控件显示），但是没有启用交互性
     * @param sliceName 交互器名称（由 AddSliceWidget 返回的名称）
     * @return 如果交互器存在且操作成功返回 true，否则返回 false
     */
    bool ShowSliceWidget(QString sliceName);
    /**
     * @brief Slice，根据SliceWidget的交互面进行截面
     * @param QString sliceWidgetName，交互器的名字
     * @return bool，如果没有打开截面交互器即没有截面参考，则返回false，否则返回true
     * @details 根据交互器进行截面（只能对交互器交互截取的面有用）
     */
    bool Slice(QString sliceWidgetName);
    /**
     * @brief SliceByXPlane(默认会锁定交互器的可交互性)
     * @param QString sliceName，传入AddSliceWidget返回的slice actor名称
     * @param double xValue,x方向的值
     * @return
     */
    bool SliceByXPlane(QString sliceName, double xValue);
    /**
     * @brief SliceByYPlane(默认会锁定交互器的可交互性)
     * @param QString sliceName，传入AddSliceWidget返回的slice actor名称
     * @param double yValue，y方向的值
     * @return
     */
    bool SliceByYPlane(QString sliceName, double yValue);
    /**
     * @brief SliceByZPlane(默认会锁定交互器的可交互性)
     * @param QString sliceName，传入AddSliceWidget返回的slice actor名称
     * @param double zValue，z方向的值
     * @return
     */
    bool SliceByZPlane(QString sliceName, double zValue);
    /**
     * @brief GetSliceOrigin
     * @param QString sliceWidgetName，截面交互器名称
     * @return QVector3D，返回截面时的原点，（只能对交互器交互截取的面有用）
     */
    QVector3D GetSliceOrigin(QString sliceWidgetName);

    /**
     * @brief GetSliceNormal
     * @param QString sliceWidgetName，截面交互器名称
     * @return QVector3D，返回截面时的法方向（只能对交互器交互截取的面有用）
     */
    QVector3D GetSliceNormal(QString sliceWidgetName);

    /****等值线/面，才会进行渲染****/
    /**
     * @brief AddContour
     * @param QString contourDerivedActor，指定这个contour由哪个actor去生成
     * @return QString，返回添加的contour的名字，注意是默认命名即“contour”+第“i”个。
     * @details 默认命名方式，此时只是指定了由哪个actor去提取新建的contour，但没有设置提取contour的其他参数。
     */
    QString AddContour(QString contourDerivedActor);

    /**
     * @brief SetContouredBy
     * @param QString contourName，输入进行设置的contourActor的名字
     * @param QString propertyName，指定用于提取等值线/面的属性
     * @return double*，返回该属性的取值区间、
     * @details 对指定的contourActor设置提取等值线/面的属性参数，同时获取此contourActor该属性的取值区间。
     */
    double* SetContouredBy(QString contourName,QString propertyName);

    /**
     * @brief AddEntry
     * @param QString contourName，输入进行操作的contourActor的名字
     * @param double value，设置提取的属性的值。需要在此之前通过SetContouredBy函数指定contourActor的提取属性。
     * @return int，返回这个entry的id，id从0开始。若在此之前没有对该contourActor指定提取属性，则会返回-1.
     * @details 对于指定的contourActor提取值为value的线/面，并返回这个entry的id。若在此之前没有通过SetContouredBy函数指定该contourActor的提取属性，则会返回-1。
     */
    int AddEntry(QString contourName,double value);
    /**
     * @brief EditEntry
     * @param QString contourName，输入进行操作的contourActor的名字
     * @param int entryId，输入进行操作的contourActor的entryid
     * @param double value，新的提取属性值大小
     * @return bool，如果该entryid无效，则返回false，否则返回true
     * @details 对指定的contourActor的第i个entry（i从0开始）修改提取等值线/面的属性值大小。
     */
    bool EditEntry(QString contourName,int entryId,double value);

    /**
     * @brief RemoveEntry
     * @param QString contourName，输入进行操作的contourActor的名字
     * @param int entryId，输入进行操作的contourActor的entryid
     * @return bool，如果该entryid无效，则返回false，否则返回true
     * @details 对指定的contourActor，移除指定的某个entry（entryid从0开始）。
     */
    bool RemoveEntry(QString contourName,int entryId);
    /****矢量图形化****/
    /**
     * @brief AddGlyph
     * @param QString glyphDerivedActor，指定glyph由哪个actor生成
     * @return  QString，返回添加的glyph的名字，注意是默认命名即“glyph”+第“i”个。
     * @details 默认参数是采样点ration1/1000，TipLength=0.1，TipRadius=0.01，ShaftRadius=0.001，ScaleFactor=0.0001
     */
    QString AddGlyph(QString glyphDerivedActor);

    /**
     * @brief SetGlyphVector
     * @param QString glyphName，需要进行操作的glyph名称
     * @param QString vectorName，指定矢量化的active向量
     * @details 设置某个glyph的active向量，即进行矢量化的属性
     */
    void SetGlyphActiveVector(QString glyphName,QString vectorName);

    /**
     * @brief SetGlyphSourceTipLength对于指定的glyph调整箭头尖的长度
     * @param QString glyphName，需要进行操作的glyph名称
     * @param double tipLength，箭头尖的长度
     */
    void SetGlyphSourceTipLength(QString glyphName, double tipLength);

    /**
     * @brief SetGlyphSourceTipRadius对于指定的glyph调整箭头尖的半径
     * @param QString glyphName，需要进行操作的glyph名称
     * @param double tipRadius，箭头尖的半径
     */
    void SetGlyphSourceTipRadius(QString glyphName, double tipRadius);

    /**
     * @brief SetGlyphSourceShaftRadius对于指定的glyph调整箭头杆的半径
     * @param QString glyphName，需要进行操作的glyph名称
     * @param double shaftRadius，箭头杆的半径
     */
    void SetGlyphSourceShaftRadius(QString glyphName,double shaftRadius);

    /**
     * @brief SetGlyphSourceScaleFactor对于指定的glyph调整factor大小
     * @param QString glyphName，需要进行操作的glyph名称
     * @param double scaleFactor，调整向量对于矢量化的影响程度
     */
    void SetGlyphSourceScaleFactor(QString glyphName, double scaleFactor);

    /**
     * @brief SetGlyphPointsNumber对于指定的glyph调整抽样点数，即显示n个点
     * @param QString glyphName，需要进行操作的glyph名称
     * @param int pointsNumber，提取n个点
     */
    void SetGlyphPointsNumber(QString glyphName,int pointsNumber);

    /****Q值计算****/
    /**
     * @brief CalculateQCriterion
     * @param QString actorName，需要去计算Q值的actor名称
     * @details 对该actor计算Q值，计算后以“QCriterion”加入该actor的属性列表并设置为活动标量，计算后可直接使用contour相关结构以便提取涡结构
     */
    void CalculateQCriterion(QString actorName);

    /*********流线****/
    /**
     * @brief AddStreamTracer，添加流线actor
     * @param QString derivedActor，指定操作对象
     * @return QString 返回流线actor名字，默认命名“StreamTrace”+第“i”个
     * @details 根据指定空间面actor，在其上做速度投影，随机选取点作为流线起点并绘制流线。默认，随机采样点ration是1000，最大点数100、双向积分、最大步数100。
     */
    QString AddStreamTracer(QString derivedActor);

    /**
     * @brief SetStreamTracerRatio，设置采样点比率和最大采样点个数
     * @param QString streamTraceActor，流线actor名字
     * @param int pointRatio，采样点比率
     * @param int maxPointNum，采样点最大个数
     * @return 如果这个actor不是流线actor则返回fasle，否则true
     */
    bool SetStreamTracerRatio(QString streamTraceActor,int pointRatio,int maxPointNum);

    /**
     * @brief SetStreamTracerDiretion，设置流线积分方向
     * @param QString streamTraceActor，流线actor名字
     * @param int flag，flag为-1则负方向积分，0双向积分，1正向积分。
     * @return 如果这个actor不是流线actor则返回fasle，如果flag是-1、0、1以外的数字返回false，否则true
     */
    bool SetStreamTracerDiretion(QString streamTraceActor,int flag);

    /**
     * @brief SetStreamTracerMaximumPropagation最大传播距离
     * @param QString streamTraceActor，流线actor名字
     * @param double maxPropagation,最大传播距离
     * @return 如果这个actor不是流线actor则返回fasle，否则true
     */
    bool SetStreamTracerMaximumPropagation(QString streamTraceActor,double maxPropagation);

    /**
     * @brief SetStreamTracerMaximumNumberOfSteps流线的最大步数
     * @param QString streamTraceActor，流线actor名字
     * @param int maxNumberOfSteps，流线可以前进的最大步数
     * @return 如果这个actor不是流线actor则返回fasle，否则true
     */
    bool SetStreamTracerMaximumNumberOfSteps(QString streamTraceActor,int maxNumberOfSteps);

    /**
     * @brief SetStreamTracerMaximumIntegrationStep
     * @param QString streamTraceActor，流线actor名字
     * @param double maxIntegrationStep，最大步长
     * @return 如果这个actor不是流线actor则返回fasle，否则true
     */
    bool SetStreamTracerMaximumIntegrationStep(QString streamTraceActor,double maxIntegrationStep);

    /**
     * @brief SetStreamTracerIntegrationStepUnit
     * @param QString streamTraceActor，流线actor名字
     * @param int unit，值LENGTH_UNIT (1)积分步长以长度单位（如米、毫米等）表示。
     * CELL_LENGTH_UNIT (2)积分步长基于网格单元的长度。
     * @return 如果这个actor不是流线actor则返回fasle，如果unit不是1或2返回false，否则true
     * @details 步长单位The valid unit is now limited to only LENGTH_UNIT (1) and CELL_LENGTH_UNIT (2), EXCLUDING the previously-supported TIME_UNIT.
     */
    bool SetStreamTracerIntegrationStepUnit(QString streamTraceActor,int unit);

    /**
     * @brief ExtracteS1
     * @param QString p1SurfaceName，周期P1的名称
     * @param QString p2SurfaceName，周期面P2的名称
     * @param QString inletName，inlet面的名称
     * @param QString outletName，周期面P2的名称
     * @param QString shroudName，周期面P2的名称
     * @param QString hubName，周期面P2的名称
     * @param QString fluidName，整个固体的名称，默认"FLUID"
     * @param double relativeR，相对百分比，默认50%
     * @return QString，返回S1面的名称，默认命名：S1_"p1SurfaceName"_RelativeR=relativeR
     * @details 仅能判断是否存在p1SurfaceName和FLUID的actor，具体是不是周期面1需要自行确认。
     */
    QString ExtractS1(QString p1SurfaceName,QString p2SurfaceName,QString inletName,QString outletName,QString shroudName,QString hubName,QString fluidName="FLUID",double relativeR = 50.0);
    /**
     * @brief ExtractS2
     * @param periodSurfaceName 周期面名称
     * @param fluidName 三维流场体数据 actor 名称，默认"FLUID"
     * @param numSteps 旋转分段数，默认10
     * @param startAngle 起始旋转角度，默认0°
     * @param endAngle 结束旋转角度，默认360°
     * @return QString 返回S2面的名称，格式为：S2_周期面名_Start=xx_End=xx_Steps=xx
     * @details 判断周期面名和FLUID是否存在，内部调用 generate_s2_surface 实现。
     */
    QString ExtractS2(QString periodSurfaceName,QString fluidName="FLUID",int numSteps=10,double startAngle=0, double endAngle=360);

    /**
     * @brief RotateAndCopyActor
     * @param actorName,要旋转复制的actor名称
     * @param copies复制分数
     * @param angle（旋转角度
     * @param axis（旋转轴，xyz，默认x轴
     * @return 旋转后所有actor的名称列表（包括原始actor）
     */
    QStringList RotateAndCopyActor(QString actorName, int copies,double angle, char axis = 'X');

    /**
     * @brief TranslateAndCopyActor
     * @param actorName,要平移复制的actor名称
     * @param copies，复制份数
     * @param dt，平移距离
     * @param dirX，方向向量x方向
     * @param dirY，方向向量y方向
     * @param dirZ，方向向量z方向
     * @return 复制生成的copies个actor名称
     * @details 移动的距离单位和原始数据中的坐标单位相同。每个方向移动的距离为dirX*dt，dirY*dt,dirZ*dt
     */
    QStringList TranslateAndCopyActor(QString actorName, int copies, double dt, double dirX, double dirY, double dirZ);

    /**
       * @brief 计算相对马赫数
       * @param actorName 需要进行计算的actor名称
       * @details 使用属性"ur", "vr", "wr"（相对速度分量）和"T"（温度）计算相对马赫数，结果以"Ma_rel"加入属性列表
       */
      void CalculateRelativeMachNumber(QString actorName);

      /**
       * @brief 计算熵
       * @param actorName 需要进行计算的actor名称
       * @details 使用属性"p"（压力）和"rho"（密度）计算熵，结果以"Entropy"加入属性列表
       * 取pref=101325 ，rhoref=1.225
       */
      void CalculateEntropy(QString actorName);

      /**
       * @brief CalculateProperty对指定actor的属性数据进行公式计算
       * @param actorName,actor名称
       * @param formula计算公式（例如 "u^1.5/w"），公式中的变量名必须与属性名称完全匹配
       * @param resultName计算结果属性名称
       * @return bool  是否成功执行计算
       * @details 注意，当输入公式出中出现非propertyname的变量名的话，程序会崩溃，这是由vtk管线决定的。推荐写入表达式的时候，让选择每个每个变量和操作符，类似计算器按钮。然后保存为字符串调用该函数
       * 其次对于常量比如e和pi，公式中需以具体数值表示，如3.1415926*T
       */
      bool CalculateProperty(QString actorName, QString formula, QString resultName);
      /**
       * @brief SaveSliceData保存切片数据到文件
       * @param sliceName切片名称（由AddSliceWidget返回的名称）
       * @param filePath保存路径（含文件名，但不含后缀，后缀是根据format自动加上的）
       * @param format,默认为1，0-vtp格式，1-dat格式
       * @return bool保存成功返回true，失败返回false
       * @details保存为ASCII格式的vtk polydata文件（.vtp），包含所有属性值
       */
      bool SaveSliceData(QString sliceName,QString filePath,int format);

//      /**
//       * @brief SaveIntersectionLineData做两个actor的交线并保存到指定路径
//       * @param actorName1曲面一的名称
//       * @param actorName2曲面二的名称
//       * @param filePath曲面一和曲面交线的写出保存路径
//       * @return bool，是否成功写出
//       */
//      bool SaveIntersectionLineData(QString actorName1, QString actorName2, QString filePath);

private:

    //共享的
    vtkMultiBlockDataSet* m_multiBlock;
    vtkUnstructuredGrid* m_unstructuredGrid;
    vtkPointData* m_pointData;
    vtkDataArray* m_scalars;

    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<QVTKInteractor> m_qvtkInteractor;

    //渲染管理actor
    //actorslist保存所有打开过的actor，每次新建就在里面，删除就挪走
    //actorsststus保存actor状态，1可见，0不可见
    std::map<std::string,vtkActor*> m_actorsList;
    std::map<std::string,bool> m_actorsStatus;  //正在显示的actor状态为true
    std::map<std::string,Contour*> m_contoursList;
    std::map<std::string,Glyph*> m_glyphsList;
    std::map<std::string,vtkSmartPointer<vtkImplicitPlaneWidget2> > m_sliceWigetList;
    std::map<std::string,vtkSmartPointer<vtkImplicitPlaneRepresentation> > m_slicePlaneRepList;
    std::map<std::string,vtkSmartPointer<vtkCutter> > m_cutterList;//既有slicewidget的生成又有plane的
    std::map<std::string,vtkSmartPointer<vtkMaskPoints> > m_streamTraceMaskPointsList;
    std::map<std::string,vtkSmartPointer<vtkStreamTracer> > m_streamTraceList;

    vtkSmartPointer<vtkOrientationMarkerWidget> m_orientationMarker;
    vtkSmartPointer<vtkAxesActor> m_axes;

    std::map<std::string,vtkActor*> m_colorLineList;//??
    std::map<std::string,bool> m_colorLineStatus;  //正在显示的actor状态为true?
    std::map<std::string,vtkSmartPointer<vtkContourFilter> > m_ColorLineContourFilterList;//?
    std::map<std::string, bool> m_sliceWidgetVisibilityStatus;

    //保存s1的面的提取
    std::map<std::string, vtkSmartPointer<vtkUnstructuredGrid>> m_clippedCache;
    int m_varNum;
    int m_contourNum = 0;
    int m_sliceWidgetNum = 0;
    int m_glyphNum = 0;
    int m_blockNum = 0;
    int m_streamTraceNum = 0;

    TecplotReader m_reader;

    //s1面相关：六个面提取一个体
    vtkSmartPointer<vtkUnstructuredGrid> clipWithSixSurfaces(
        vtkSmartPointer<vtkUnstructuredGrid> input,
        vtkSmartPointer<vtkUnstructuredGrid> periodic1,
        vtkSmartPointer<vtkUnstructuredGrid> periodic2,
        vtkSmartPointer<vtkUnstructuredGrid> inlet,
        vtkSmartPointer<vtkUnstructuredGrid> outlet,
        vtkSmartPointer<vtkUnstructuredGrid> shroud,
        vtkSmartPointer<vtkUnstructuredGrid> hub);
    //根据中期面划分连接性
    vtkSmartPointer<vtkUnstructuredGrid> ExtractConnectedRegionWithP1(
        vtkSmartPointer<vtkUnstructuredGrid>inputGrid,
        vtkSmartPointer<vtkUnstructuredGrid> p1Data);

    //旋转复制
    vtkSmartPointer<vtkTransform> CreateRotationTransform(char axis, double angle);

    //全局颜色映射成员
    vtkSmartPointer<vtkLookupTable> m_globalLUT;         // 全局颜色查找表
    vtkSmartPointer<vtkScalarBarActor> m_globalScalarBar; // 全局颜色条
    std::string m_globalActiveProperty;                   // 当前激活的属性名（如"p"）
    bool m_globalColorMapOn;                              // 全局颜色映射开关
    int m_globalNumberOfColors;                          // 全局色阶数量
    bool m_globalBarOn;
    vtkDataSet* GetActorDataSet(const std::string& actorName);//获取actor对应数据集
    bool GetGlobalPropertyRange(double range[2]);
    void UpdateAllActorsColorMapping();

    // 全局等值线相关
    bool m_globalColorLineOn; // 全局等值线开关
    vtkSmartPointer<vtkContourFilter> m_globalContourFilter; // 全局等值线过滤器
    vtkSmartPointer<vtkActor> m_globalContourActor; // 全局等值线actor
    void UpdateGlobalContourLines();

};

#endif // TECPLOTWIDGET_H
