#include "TecplotWidget.h"
//#include<vtkPSphereSource.h>
#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);
//VTK_MODULE_INIT(vtkRenderingContextOpenGL2);
VTK_MODULE_INIT(vtkRenderingFreeType)
TecplotWidget::TecplotWidget(QWidget *parent)
    :QVTKOpenGLNativeWidget(parent)
{
    // 初始化渲染器和渲染窗口等所有功能共享部分
    this->m_multiBlock=vtkSmartPointer<vtkMultiBlockDataSet>::New();
    this->m_unstructuredGrid=vtkSmartPointer<vtkUnstructuredGrid>::New();
    this->m_pointData=vtkSmartPointer<vtkPointData>::New();
    this->m_varNum = 0;

    this->m_renderer = vtkSmartPointer<vtkRenderer>::New();
    this->m_renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    this->m_renderWindow->AddRenderer(m_renderer);
    this->setRenderWindow(m_renderWindow);
    QColor lightBlue(82, 87, 110);
    this->m_renderer->SetBackground(lightBlue.redF(), lightBlue.greenF(), lightBlue.blueF());
    //获取交互器
    this->m_qvtkInteractor=this->interactor();

    //坐标
    m_axes = vtkSmartPointer<vtkAxesActor>::New();
    m_orientationMarker = vtkSmartPointer<vtkOrientationMarkerWidget>::New();

    m_orientationMarker->SetOrientationMarker(m_axes);
    m_orientationMarker->SetInteractor(m_renderWindow->GetInteractor());
    // m_orientationMarker->SetViewport(0.0, 0.0, 0.3, 0.3);
    m_orientationMarker->SetViewport(0.0, 0.0, 0.12, 0.4);
    m_orientationMarker->SetEnabled(1);
    m_orientationMarker->InteractiveOff();

    m_renderer->SetBackground2(1.0, 1.0, 1.0); // 设置页面底部颜色值
    m_renderer->SetBackground(0.529, 0.8078, 0.92157); // 设置页面顶部颜色值
    m_renderer->SetGradientBackground(true); // 开启渐变色背景设置
}
TecplotWidget::~TecplotWidget()
{
}
/**设置需要读入的文件的路径，对block0进行绘制？？**/
void TecplotWidget::SetFileName(QString fileName)
{
    this->m_multiBlock = this->m_reader.ReadTecplotData(fileName.toStdString());
    this->m_blockNum = this ->m_multiBlock->GetNumberOfBlocks();

    //将每个block作为一个actor渲染，并且以block的name来命名
    std::string blockName = "";
    for(int i = 0;i < this->m_blockNum;i++)
    {
        const char* Name = this->m_multiBlock->GetMetaData(i)->Get(vtkCompositeDataSet::NAME());
        blockName = Name;

        auto tmpMapper=vtkSmartPointer<vtkDataSetMapper>::New();
        auto tmpActor=vtkSmartPointer<vtkActor>::New();
        m_actorsList[blockName]=tmpActor;
        this->m_actorsStatus[blockName]=true;
        tmpMapper->SetInputData(vtkUnstructuredGrid::SafeDownCast(this->m_multiBlock->GetBlock(i)));
        tmpActor->SetMapper(tmpMapper);
        this->m_renderer->AddActor(tmpActor);
        blockName.clear();
    }
    this->m_renderWindow->Render();

}
int TecplotWidget::GetNumberOfBlock()
{
    if(this->m_multiBlock->GetNumberOfBlocks()==0)
    {
        QMessageBox::warning(this, "Warning", "请先打开文件");
        return 0;
    }
    return this->m_multiBlock->GetNumberOfBlocks();
}
QStringList TecplotWidget::GetActorList()
{
    QStringList actorNameList;
    for (const auto& pair : m_actorsList)
    {
        actorNameList << QString::fromStdString(pair.first);
    }
    return actorNameList;
}
bool TecplotWidget::ActorVisibilityOn(QString actorName)
{
    std::string name = actorName.toStdString();
    if(this->m_actorsList.count(name)==0)
    {
        return false;
    }
    if(actorName.contains("Slice"))
    {
        this->m_sliceWigetList[name]->EnabledOn();
    }
    if(this->m_barsStatus.count(name)!=0){
        if(this->m_barsStatus[name])
        {
            this->m_barsList[name]->VisibilityOn();
        }
    }
    this->m_actorsStatus[name] = true;
    vtkActor* objActor = this->m_actorsList[name];
    objActor->VisibilityOn();
    this->m_renderWindow->Render();
    return true;
}
bool TecplotWidget::ActorVisibilityOff(QString actorName)
{
    std::string name = actorName.toStdString();
    if(this->m_actorsList.count(name)==0)
    {
        return false;
    }
    if(actorName.contains("Slice"))
    {
        this->m_sliceWigetList[name]->EnabledOff();
    }
    if(this->m_barsList.count(name)!=0)
    {
        this->m_barsList[name]->VisibilityOff();
        this->m_barsStatus[name] = false;
    }
    this->m_actorsStatus[name] = false;
    vtkActor* objActor = this->m_actorsList[name];
    objActor->VisibilityOff();
    this->m_renderWindow->Render();
    return true;
}
bool TecplotWidget::RemoveActor(QString actorName)
{
    std::string name = actorName.toStdString();
    if(this->m_actorsList.count(name)==0)
    {
        return false;
    }
    if(this->m_barsList.count(name)!=0)
    {
        auto barActor = this->m_barsList[name];
        this->m_renderer->RemoveActor(barActor);
        this->m_barsList.erase(name);
        this->m_barsStatus.erase(name);
    }
    if(this->m_lutsList.count(name)!=0)
    {
        this->m_lutsList.erase(name);
    }
    if(actorName.contains("Slice"))
    {
        this->m_cutterList.erase(name);
        this->m_sliceWigetList.erase(name);
        this->m_slicePlaneRepList.erase(name);
        this->m_sliceWidgetNum--;
    }else if(actorName.contains("StreamTracer"))
    {
        this->m_streamTraceNum--;
        this->m_streamTraceList.erase(name);
        this->m_streamTraceMaskPointsList.erase(name);
    }else if(actorName.contains("Glyph"))
    {
        vtkActor* objActor = this->m_actorsList[name];
        this->m_renderer->RemoveActor(objActor);
        this->m_actorsList.erase(name);
        this->m_actorsStatus.erase(name);
        this->m_glyphNum--;
        Glyph* tmp = this->m_glyphsList[name];
        delete tmp;
        this->m_glyphsList.erase(name);
        this->m_renderWindow->Render();
        return true;
    }else if(actorName.contains("Contour"))
    {
        vtkActor* objActor = this->m_actorsList[name];
        this->m_renderer->RemoveActor(objActor);
        this->m_actorsList.erase(name);
        this->m_actorsStatus.erase(name);
        this->m_contourNum--;
        Contour* tmp = this->m_contoursList[name];
        delete tmp;
        this->m_contoursList.erase(name);
        this->m_renderWindow->Render();
        return true;
    }

    vtkActor* objActor = this->m_actorsList[name];
    this->m_renderer->RemoveActor(objActor);
    this->m_actorsList.erase(name);
    this->m_actorsStatus.erase(name);
    this->m_renderWindow->Render();
    return true;
}
int TecplotWidget::GetNumberOfProperty(QString actorName)
{
    vtkActor* objActor = this->m_actorsList[actorName.toStdString()];
    vtkMapper* mapper= objActor->GetMapper();
    vtkDataSet* dataSet = vtkDataSet::SafeDownCast(mapper->GetInput());
    int num = dataSet->GetPointData()->GetNumberOfArrays();
    for(int i = 0;i < num ;i++)
    {
        cout <<endl <<dataSet->GetPointData()->GetArrayName(i);
    }
    return num;
}
QStringList TecplotWidget::GetPropertyList(QString actorName)
{
    vtkActor* objActor = this->m_actorsList[actorName.toStdString()];
    vtkMapper* mapper= objActor->GetMapper();
    vtkDataSet* dataSet = vtkDataSet::SafeDownCast(mapper->GetInput());

    QStringList propertyList;
    int num = dataSet->GetPointData()->GetNumberOfArrays();
    for(int i = 0;i < num;i++)
    {
        propertyList<<dataSet->GetPointData()->GetArrayName(i);
    }
    return propertyList;
}
QString TecplotWidget::GetPropertyName(QString actorName,int id)
{//??没测试return啊
    vtkActor* objActor = this->m_actorsList[actorName.toStdString()];
    vtkMapper* mapper= objActor->GetMapper();
    vtkDataSet* dataSet = vtkDataSet::SafeDownCast(mapper->GetInput());
    auto propertyName = dataSet->GetPointData()->GetArrayName(id);
    //QString name(propertyName);
    return QString::fromUtf8(propertyName);
}
void TecplotWidget::SetBackgroundColor(QColor color)
{
    int r=color.red();
    int g=color.green();
    int b=color.blue();
    double normalizedR = r / 255.0;
    double normalizedG = g / 255.0;
    double normalizedB = b / 255.0;

    m_renderer->SetBackground(normalizedR, normalizedG, normalizedB);
    //？？是否每次都要设置后都要重新渲染
    m_renderWindow->Render();
}
QColor TecplotWidget::GetBackgroundColor()
{
    double* rgb = m_renderer->GetBackground();
    int r = static_cast<int>(rgb[0] * 255);
    int g = static_cast<int>(rgb[1] * 255);
    int b = static_cast<int>(rgb[2] * 255);
    return QColor(r, g, b);
}
void TecplotWidget::SetSolidColor(QString actorName,QColor color)
{
    vtkActor* objActor = this->m_actorsList[actorName.toStdString()];
    if(this->m_barsList.count(actorName.toStdString())!=0)
    {//如果之前执行过颜色映射，需要关闭颜色映射、设置bar不可见
        objActor->GetMapper()->ScalarVisibilityOff();
        this->m_barsList[actorName.toStdString()]->VisibilityOff();
        this->m_barsStatus[actorName.toStdString()]= false;
    }
    int r=color.red();
    int g=color.green();
    int b=color.blue();
    double normalizedR = r / 255.0;
    double normalizedG = g / 255.0;
    double normalizedB = b / 255.0;
    objActor->GetProperty()->SetColor(normalizedR,normalizedG,normalizedB);
    m_renderWindow->Render();
}
QColor TecplotWidget::GetSolidColor(QString actorName)
{
    vtkActor* objActor = this->m_actorsList[actorName.toStdString()];
    double* rgb=objActor->GetProperty()->GetColor();
    return QColor::fromRgbF(rgb[0],rgb[1],rgb[2]);
}
bool TecplotWidget::SetColorMapOn(QString actorName,QString propertyName)
{
    std::string objName = actorName.toStdString();
    std::string objVar = propertyName.toStdString();
    vtkActor* objActor = this->m_actorsList[objName];
    vtkMapper* mapper= objActor->GetMapper();
    vtkDataSet* dataSet = vtkDataSet::SafeDownCast(mapper->GetInput());
    //如果采用默认参数，则必须之前打开过颜色映射
    if(this->m_barsList.count(objName)==0)
    {
        if(propertyName=="") return false;
        //该actor第一次打开颜色映射，需要新建lut
        vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
        auto barActor = vtkSmartPointer<vtkScalarBarActor>::New();
        this->m_lutsList[objName] = lut;
        this->m_barsList[objName] = barActor;
        this->m_renderer->AddActor2D(barActor);
    }
    auto lut = m_lutsList[objName];
    auto barActor = m_barsList[objName];
    this->m_barsStatus[objName] = true;
    if(propertyName=="")
    {
        mapper->ScalarVisibilityOn();
        barActor->VisibilityOn();
        this->m_renderWindow->Render();
        return true;
    }
    // 设置 Active Scalars
    int selectedId = dataSet->GetPointData()->SetActiveScalars(objVar.c_str());
    vtkDataArray* scalar = dataSet->GetPointData()->GetArray(selectedId);
    lut->SetTableRange(scalar->GetRange());
    lut->SetNumberOfColors(256);
    lut->Build();
    barActor->SetLookupTable(lut);
    mapper->SetScalarRange(scalar->GetRange());
    mapper->SetLookupTable(lut);

    mapper->ScalarVisibilityOn();
    barActor->VisibilityOn();
    m_renderWindow->Render();
    return true;
}
bool TecplotWidget::SetColorMapOff(QString actorName)
{
    if(this->m_actorsList.count(actorName.toStdString())==0)
        return false;
    vtkActor* objActor = this->m_actorsList[actorName.toStdString()];

    if(this->m_barsList.count(actorName.toStdString())!=0)
    {//如果之前执行过颜色映射，需要关闭颜色映射、设置bar不可见
        objActor->GetMapper()->ScalarVisibilityOff();
        this->m_barsList[actorName.toStdString()]->VisibilityOff();
        this->m_barsStatus[actorName.toStdString()] = false;
    }
    m_renderWindow->Render();
    return true;
}

QString TecplotWidget::AddSliceWidget(QString derivedActorName)
{
    this->m_sliceWidgetNum++;
    std::string name = "Slice"+std::to_string(m_sliceWidgetNum);
    vtkActor* objActor = this->m_actorsList[derivedActorName.toStdString()];
    vtkDataSet* data = vtkDataSet::SafeDownCast(objActor->GetMapper()->GetInput());

    vtkSmartPointer<vtkImplicitPlaneWidget2> cutPlaneWidget = vtkSmartPointer<vtkImplicitPlaneWidget2>::New();
    this->m_sliceWigetList[name] = cutPlaneWidget;
    vtkSmartPointer<vtkImplicitPlaneRepresentation> cutPlaneRep = vtkSmartPointer<vtkImplicitPlaneRepresentation>::New();
    this->m_slicePlaneRepList[name] = cutPlaneRep;
    vtkSmartPointer<vtkCutter> cutter = vtkSmartPointer<vtkCutter>::New();
    this->m_cutterList[name] = cutter;
    cutter->SetInputData(data);

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    vtkSmartPointer<vtkActor> cutActor = vtkSmartPointer<vtkActor>::New();
    cutActor->SetMapper(mapper);
    this->m_actorsList[name] = cutActor;
    this->m_actorsStatus[name] = true;
    this->m_renderer->AddActor(cutActor);

    cutPlaneRep->SetPlaceFactor(1.0);
    cutPlaneRep->PlaceWidget(data->GetBounds());
    cutPlaneRep->SetEdgeColor(1.0,0.0,0.0);
    cutPlaneWidget->SetInteractor(this->m_qvtkInteractor);
    cutPlaneWidget->SetRepresentation(cutPlaneRep);
    cutPlaneWidget->On();
    return QString::fromStdString(name);
}
bool TecplotWidget::Slice(QString sliceWidgetName)
{
    std::string name = sliceWidgetName.toStdString();
    if(this->m_sliceWigetList.count(name)==0)
        return false;
    vtkImplicitPlaneRepresentation* rep = this->m_slicePlaneRepList[name];
    vtkCutter* cutter = this->m_cutterList[name];
    vtkActor* actor = this->m_actorsList[name];

    vtkSmartPointer<vtkPlane> cutPlane = vtkSmartPointer<vtkPlane>::New();
    rep->GetPlane(cutPlane);
    cutter->SetCutFunction(cutPlane);
    cutter->Update();
    actor->GetMapper()->SetInputConnection(cutter->GetOutputPort());
    this->m_renderWindow->Render();
    return true;
}
QVector3D TecplotWidget::GetSliceOrigin(QString sliceWidgetName)
{
    std::string name = sliceWidgetName.toStdString();
    if(this->m_sliceWigetList.count(name)==0)
        std::cerr<<"error";
    vtkImplicitPlaneRepresentation* rep = this->m_slicePlaneRepList[name];
    double origin[3];
    rep->GetNormal(origin);

    QVector3D sliceNormal(static_cast<float>(origin[0]),static_cast<float>(origin[1]),static_cast<float>(origin[2]));
    return sliceNormal;
}
QVector3D TecplotWidget::GetSliceNormal(QString sliceWidgetName)
{
    std::string name = sliceWidgetName.toStdString();
    if(this->m_sliceWigetList.count(name)==0)
        std::cerr<<"error";
    vtkImplicitPlaneRepresentation* rep = this->m_slicePlaneRepList[name];
    double normal[3];
    rep->GetNormal(normal);

    QVector3D sliceNormal(static_cast<float>(normal[0]),static_cast<float>(normal[1]),static_cast<float>(normal[2]));
    return sliceNormal;
}

QString TecplotWidget::AddContour(QString contourDerivedActor)
{
    //默认命名方式,此时只是完成了contour的搭建和管理，但是并未设置某个contour的具体值
    this->m_contourNum++;
    std::string contourName = "Contour" + std::to_string(this->m_contourNum);
    Contour* contour = new Contour();
    this->m_contoursList[contourName] = contour; //一个名字contourname对应一个vtkcontour指针去管理
    std::string derivedName = contourDerivedActor.toStdString();
    contour->Initialize(contourName,derivedName,this->m_actorsList,this->m_renderer);
    this->m_actorsStatus[contourName] = true;
    QString name = QString::fromStdString(contourName);
    return name;
}
double* TecplotWidget::SetContouredBy(QString contourName,QString propertyName)
{
    Contour* contourptr = this->m_contoursList[contourName.toStdString()];
    double* range = contourptr->SetActiveProperty(propertyName.toStdString());
    return range;
}
int TecplotWidget::AddEntry(QString contourName, double value)
{
    Contour* contourptr = this->m_contoursList[contourName.toStdString()];
    int entryId =contourptr->AddEntry(value);
    return entryId;
}
bool TecplotWidget::EditEntry(QString contourName, int entryId, double value)
{
    Contour* contourptr = this->m_contoursList[contourName.toStdString()];
    bool flag = contourptr->EditEntry(entryId,value);
    return flag;
}
bool TecplotWidget::RemoveEntry(QString contourName, int entryId)
{
    //注意，remove一个entry后，该entry后面的所有id都要往前挪一位
    Contour* contourptr = this->m_contoursList[contourName.toStdString()];
    bool flag = contourptr->RemoveEntry(entryId);
    return flag;
}
/*void TecplotWidget::ShowContour(QString contourName)
{
    //注意apply之后，默认会关闭其他所有的actor显示
    for(auto& object:this->m_actorsList)
    {//打开的其他actor关闭
        if(object.first!=contourName.toStdString()){
            vtkActor* actor = object.second;
            actor->VisibilityOff();
            m_actorsStatus[object.first] = false;
        }
    }
    this->m_renderer->Render();
}*/
/****矢量图形化****/
QString TecplotWidget::AddGlyph(QString glyphDerived)
{
    //默认命名方式,此时只是完成了contour的搭建和管理，但是并未设置某个contour的具体值
    this->m_glyphNum++;
    std::string glyphName = "Glyph" + std::to_string(this->m_glyphNum);
    Glyph* glyph = new Glyph();
    this->m_glyphsList[glyphName] = glyph;//指针管理
    std::string derivedName = glyphDerived.toStdString();
    glyph->Initialize(glyphName,derivedName,this->m_actorsList,this->m_renderer);
    //此时，actorslist里添加了这个contourname对应的actor
    this->m_actorsStatus[glyphName] = true;
    QString name = QString::fromStdString(glyphName);
    return name;
}
void TecplotWidget::SetGlyphActiveVector(QString glyphName,QString vectorName)
{
    Glyph* glyphptr = this->m_glyphsList[glyphName.toStdString()];
    glyphptr->SetGlyphVector(vectorName.toStdString());
    this->m_renderWindow->Render();
}
void TecplotWidget::SetGlyphSourceTipLength(QString glyphName, double tipLength)
{
    //箭头尖的长度
    Glyph* glyphptr = this->m_glyphsList[glyphName.toStdString()];
    glyphptr->SetGlyphSourceTipLength(tipLength);
    this->m_renderWindow->Render();
}

void TecplotWidget::SetGlyphSourceTipRadius(QString glyphName, double tipRadius)
{
    //箭头尖的半径
     Glyph* glyphptr = this->m_glyphsList[glyphName.toStdString()];
     glyphptr->SetGlyphSourceTipRadius(tipRadius);
     this->m_renderWindow->Render();
}
void TecplotWidget::SetGlyphSourceShaftRadius(QString glyphName,double shaftRadius)
{
    // 箭头杆的半径
    Glyph* glyphptr = this->m_glyphsList[glyphName.toStdString()];
    glyphptr->SetGlyphSourceShaftRadius(shaftRadius);
    this->m_renderWindow->Render();
}
void TecplotWidget::SetGlyphSourceScaleFactor(QString glyphName, double scaleFactor)
{
    //factor大小
    Glyph* glyphptr = this->m_glyphsList[glyphName.toStdString()];
    glyphptr->SetGlyphSourceScaleFactor(scaleFactor);
    this->m_renderWindow->Render();
}
void TecplotWidget::SetGlyphPointsNumber(QString glyphName,int pointsNumber)
{
    Glyph* glyphptr = this->m_glyphsList[glyphName.toStdString()];
    glyphptr->SetGlyphPointsNumber(pointsNumber);
    this->m_renderWindow->Render();
}

void TecplotWidget::CalculateQCriterion(QString actorName)
{
    //只进行了计算并将Q值设置为活动标量，如果要绘制涡结构，需要在获得Q值后去contour
    auto objActor = this->m_actorsList[actorName.toStdString()];
    auto data = vtkDataSet::SafeDownCast(objActor->GetMapper()->GetInput());//找到这个数据，然后对每个点做计算
    //计算梯度，得到的结果理由Gradients这个梯度array
    auto gradientFilter = vtkSmartPointer<vtkGradientFilter>::New();
    gradientFilter->SetInputData(data);
    gradientFilter->SetInputScalars(vtkDataObject::FIELD_ASSOCIATION_POINTS, "velocity");
    gradientFilter->Update();
    //下面是逐个点计算Q值
    int pointNum = data->GetNumberOfPoints();
    auto QCriterion = vtkSmartPointer<vtkDoubleArray>::New();
    QCriterion->SetNumberOfComponents(1);
    QCriterion->SetNumberOfTuples(pointNum);
    QCriterion->SetName("QCriterion");
    auto gradientsArray = gradientFilter->GetOutput()->GetPointData()->GetArray("Gradients");
    for (vtkIdType i = 0; i < pointNum; ++i)
    {
        double grad[9];
        double result;
        // 获取当前点的梯度
        gradientsArray->GetTuple(i, grad);
        // 计算Q
        result = 0.5 * (-grad[0] * grad[0] - grad[4] * grad[4] - grad[8] * grad[8] - 2 * grad[2] * grad[6] - 2 * grad[5] * grad[7] - 2 * grad[1] * grad[3]);
        // 将投影结果存储在数组中
        QCriterion->SetValue(i, result);
    }
    // 将Q加入pointdata并设置为活动标量
    data->GetPointData()->AddArray(QCriterion);
    data->GetPointData()->SetActiveScalars("QCriterion");
}
QString TecplotWidget::AddStreamTracer(QString derivedActor)
{
    this->m_streamTraceNum++;
    std::string name = "StreamTracer"+std::to_string(this->m_streamTraceNum);
    vtkActor* objActor = this->m_actorsList[derivedActor.toStdString()];
    vtkDataSet* data = vtkDataSet::SafeDownCast(objActor->GetMapper()->GetInput());
    vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
    if(data->IsA("vtkUnstructuredGrid"))
    {
        vtkSmartPointer<vtkGeometryFilter> geoFilter = vtkSmartPointer<vtkGeometryFilter>::New();
        geoFilter->SetInputData(data);
        geoFilter->Update();
        polyData=geoFilter->GetOutput();
    }else if(data->IsA("vtkPolyData"))
    {
        polyData = vtkPolyData::SafeDownCast(data);
    }
    vtkSmartPointer<vtkPolyDataNormals> normalsFilter = vtkSmartPointer<vtkPolyDataNormals>::New();
    normalsFilter->SetInputData(polyData);
    normalsFilter->ComputePointNormalsOn();
    normalsFilter->ConsistencyOn();
    normalsFilter->SplittingOff();  // 禁止切割几何
    normalsFilter->NonManifoldTraversalOff();  // 关闭非流形拓扑处理
    normalsFilter->Update();
    auto normals = normalsFilter->GetOutput()->GetPointData()->GetNormals();
    //取出速度，做速度投影
    int pointNum = data->GetNumberOfPoints();
    vtkDataArray* velocityArray;
    if(data->GetPointData()->HasArray("vel"))
    {
        velocityArray = data->GetPointData()->GetArray("vel");
    }else
    {
        velocityArray = data->GetPointData()->GetArray("velocity");
    }
    auto projectedVelocity = vtkSmartPointer<vtkDoubleArray>::New();/*设置矢量*/
    projectedVelocity->SetNumberOfComponents(3);
    projectedVelocity->SetNumberOfTuples(pointNum);
    projectedVelocity->SetName("projectedVelocity");
    for (vtkIdType i = 0; i < pointNum; ++i)
    {
        double velocity[3];
        double normal[3];
        double projected[3];
        // 获取当前点的速度和法向量
        velocityArray->GetTuple(i, velocity);
        normals->GetTuple(i, normal);
        double dotProduct = vtkMath::Dot(velocity, normal);
        for (int j = 0; j < 3; ++j) {
            projected[j] = velocity[j] - dotProduct * normal[j];
            //cout << projected[j] << " ";
        }
        //cout << endl;
        // 将投影结果存储在数组中
        projectedVelocity->SetTuple(i, projected);
    }
    data->GetPointData()->AddArray(projectedVelocity);
    data->GetPointData()->SetActiveVectors("projectedVelocity");
    //polyData->GetPointData()->AddArray(normals);
    vtkSmartPointer<vtkMaskPoints> maskPoints = vtkSmartPointer<vtkMaskPoints>::New();
    this->m_streamTraceMaskPointsList[name] =maskPoints;
    maskPoints->SetInputData(data);
    maskPoints->SetOnRatio(100); // Select every 10th point
    maskPoints->RandomModeOn(); // Enable random selection
    maskPoints->SetMaximumNumberOfPoints(1000); // Set maximum number of seed points
    maskPoints->Update();
    //test 流线源点
    //auto sphereSource=vtkSmartPointer<vtkSphereSource>::New();
    //sphereSource->SetRadius(0.0005);
    //sphereSource->Update();
    //auto sourceGlyph=vtkSmartPointer<vtkGlyph3D>::New();
    //sourceGlyph->SetInputConnection(maskPoints->GetOutputPort());
    //sourceGlyph->SetSourceConnection(sphereSource->GetOutputPort());
    //auto sphereMapper=vtkSmartPointer<vtkPolyDataMappeer>::New();
    //sphereMapper->SetInputConnection(sourceGlyph->GetOutputPort());
    //auto sphereActor=vtkSmartPointer<vtkActor>::New();
    //sphereActor->SetMapper(sphereMapper);
    //this->m_renderer->AddActor(sphereActor);
    vtkSmartPointer<vtkStreamTracer> streamTracer = vtkSmartPointer<vtkStreamTracer>::New();
    this->m_streamTraceList[name]=streamTracer;
    streamTracer->SetInputData(data); // 使用计算过的 PolyData 作为输入
    streamTracer->SetSourceConnection(maskPoints->GetOutputPort());
    streamTracer->SetIntegrationDirectionToBoth(); // 设置流线的方向
    streamTracer->SetMaximumPropagation(100); // 设置流线的最大长度
    streamTracer->SetMaximumNumberOfSteps(1000);
    streamTracer->SetIntegrator(vtkSmartPointer<vtkRungeKutta4>::New());
    streamTracer->SetComputeVorticity(true); // 计算旋度
    // 映射流线到图形管道
    vtkSmartPointer<vtkPolyDataMapper> streamlineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    streamlineMapper->SetInputConnection(streamTracer->GetOutputPort());
    vtkSmartPointer<vtkActor> streamlineActor = vtkSmartPointer<vtkActor>::New();
    streamlineActor->SetMapper(streamlineMapper);
    //streamlineActor->GetProperty()->BackfaceCullingOff();
    streamlineActor->GetProperty()->LightingOff();


    //test propertylist
//    auto streamData =streamTracer->GetOutput()->GetPointData();
//    int tmp=streamData->GetNumberOfArrays();
//    qInfo()<<"streamData num of pointdata array"<<tmp;
//    qInfo()<<"array name:";
//    for(int i =0;i<tmp;i++)
//    {
//        qInfo()<<streamData->GetArrayName(i);
//    }

    //test z-fighting问题
    //streamlineActor->GetProperty()->SetLineWidth(3.0);
    //streamlineActor->GetProperty()->SetColor(1,0,0);  //？？还是有一部分是黑色的
    streamlineMapper->SetResolveCoincidentTopologyToPolygonOffset();
    //streamlineMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(2.0,1);//z-fighting没用
    streamlineMapper->ScalarVisibilityOff(); //？？不关闭颜色映射的时候是：蓝色+黑色，关闭了流线是白色+黑色
    this->m_renderer->AddActor(streamlineActor);
    this->m_actorsList[name]= streamlineActor;
    this->m_actorsStatus[name]=true;
    this->m_renderWindow->Render();
    return QString::fromStdString(name);
}
bool TecplotWidget::SetStreamTracerRatio(QString streamTraceActor,int pointRatio,int maxPointNum)
{
    std::string name = streamTraceActor.toStdString();
    if(this->m_streamTraceList.count(name)==0)
        return false;
    vtkMaskPoints* maskPoints = this->m_streamTraceMaskPointsList[name];
    maskPoints->SetOnRatio(pointRatio);
    maskPoints->SetMaximumNumberOfPoints(maxPointNum);
    maskPoints->Modified();
    maskPoints->Update();
    this->m_renderWindow->Render();
    return true;
}
bool TecplotWidget::SetStreamTracerDiretion(QString streamTraceActor,int flag)
{
    std::string name = streamTraceActor.toStdString();
    if(this->m_streamTraceList.count(name)==0)
        return false;
    vtkStreamTracer* tracer = this->m_streamTraceList[name];
    switch(flag)
    {
    case -1:
            tracer->SetIntegrationDirectionToBackward();
        break;
    case 0:
        tracer->SetIntegrationDirectionToBoth();
        break;
    case 1:
        tracer->SetIntegrationDirectionToForward();
        break;
    default:
        return false;
    }
    tracer->Modified();
    tracer->Update();
    this->m_renderWindow->Render();
    return true;
}
bool TecplotWidget::SetStreamTracerMaximumPropagation(QString streamTraceActor,double maxPropagation)
{
    std::string name = streamTraceActor.toStdString();
    if(this->m_streamTraceList.count(name)==0)
        return false;
    vtkStreamTracer* tracer = this->m_streamTraceList[name];
    tracer->SetMaximumPropagation(maxPropagation);
    tracer->Modified();
    tracer->Update();
    this->m_renderWindow->Render();
    return true;
}
bool TecplotWidget::SetStreamTracerMaximumNumberOfSteps(QString streamTraceActor,int maxNumberOfSteps)
{
    std::string name = streamTraceActor.toStdString();
    if(this->m_streamTraceList.count(name)==0)
        return false;
    vtkStreamTracer* tracer = this->m_streamTraceList[name];
    tracer->SetMaximumNumberOfSteps(maxNumberOfSteps);
    tracer->Modified();
    tracer->Update();
    this->m_renderWindow->Render();
    return true;
}
bool TecplotWidget::SetStreamTracerMaximumIntegrationStep(QString streamTraceActor,double maxIntegrationStep)
{
    std::string name = streamTraceActor.toStdString();
    if(this->m_streamTraceList.count(name)==0)
        return false;
    vtkStreamTracer* tracer = this->m_streamTraceList[name];
    tracer->SetMaximumIntegrationStep(maxIntegrationStep);
    tracer->Modified();
    tracer->Update();
    this->m_renderWindow->Render();
    return true;
}
bool TecplotWidget::SetStreamTracerIntegrationStepUnit(QString streamTraceActor,int unit)
{
    std::string name = streamTraceActor.toStdString();
    if(this->m_streamTraceList.count(name)==0)
        return false;
    if(unit!=1&&unit!=2) return false;
    vtkStreamTracer* tracer = this->m_streamTraceList[name];
    tracer->SetIntegrationStepUnit(unit);
    tracer->Modified();
    tracer->Update();
    this->m_renderWindow->Render();
    return true;
}
/***************************************************************************
 ***************************************************************************
 ***************************************************************************
 ***********************class CutPlane的实现*********************************
 ***************************************************************************
 ***************************************************************************
 ***************************************************************************/
bool CutPlane::GetInputDataStatus()
{
    return this->dataStatus;
}
void CutPlane::AddCutPlaneWiget(std::string derivedActorName,std::map<std::string,vtkActor*>& actorsList,QVTKInteractor* qvtkInteractor,vtkRenderer* renderer)
{

}
void CutPlane::SetInputData(vtkUnstructuredGrid *inputData, QVTKInteractor *qvtkInteractor, vtkRenderer *renderer)
{
    this->ug = inputData;
    this->qvtkInteractor=qvtkInteractor;
    this->renderer=renderer;
    this->dataStatus=true;
}
void CutPlane::SetCutPlaneWidget()
{
    if(!this->dataStatus){
        std::cerr << "Please set input data first." << std::endl;
        return;
    }
    //第一次打开widget需要新建widget和plane样例,设置并绑定
    if(!this->widgetStatus)
    {
        this->cutPlaneWidget = vtkSmartPointer<vtkImplicitPlaneWidget2>::New();
        this->cutPlaneRep = vtkSmartPointer<vtkImplicitPlaneRepresentation>::New();

        this->cutPlaneRep->SetPlaceFactor(1.0);
        this->cutPlaneRep->PlaceWidget(this->ug->GetBounds());
        //this->cutPlaneRep->SetHandleSize(100);
        //this->cutPlaneRep->SetHandleColor(1.0,0.0,0.0);显示vtk8.2没有这个成员函数好像
        this->cutPlaneRep->SetEdgeColor(1.0,0.0,0.0);
        this->cutPlaneWidget->SetInteractor(this->qvtkInteractor);
        this->cutPlaneWidget->SetRepresentation(this->cutPlaneRep);
        //this->cutPlaneWidget->SetRepresentationToSurface();
        //this->cutPlaneWidget->SetResolution(1000);
        //this->cutPlaneWidget->SetRepresentationToOutline();
        //激活widget
        this->cutPlaneWidget->On();

        this->widgetStatus = true;
    }
    this->cutPlaneWidget->SetEnabled(1);
    //需要考虑每次打开widget是否得归位
}
void CutPlane::CloseCutPlaneWidget()
{
    this->cutPlaneWidget->SetEnabled(0);
}
void CutPlane::DeleteCutPlaneWidget()
{
    this->cutPlaneWidget->Off();
    this->widgetStatus = false;
}
void CutPlane::AddCutPlane(std::string &name, std::map<std::string, vtkActor *> &actorsList, std::map<std::string, bool> &actorsStatus)
{
    //如果未输入命名，则默认命名
    if(name==""){
        cutActorNum++;
        name="Slice"+std::to_string(cutActorNum);
    }
    vtkSmartPointer<vtkPlane> cutPlane = vtkSmartPointer<vtkPlane>::New();
    vtkSmartPointer<vtkCutter> cutter = vtkSmartPointer<vtkCutter>::New();
    vtkSmartPointer<vtkPolyDataMapper> cutMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    vtkSmartPointer<vtkActor> cutActor = vtkSmartPointer<vtkActor>::New();
    this->cutPlaneRep->GetPlane(cutPlane);
    cutter->SetCutFunction(cutPlane);
    cutter->SetInputData(this->ug);
    cutter->Update();
    cutMapper->SetInputConnection(cutter->GetOutputPort());
    cutActor->SetMapper(cutMapper);
    this->renderer->AddActor(cutActor);

    //在将新的actor添加进actorlist前，默认关闭其他所有actor的显示
    for(auto& pair: actorsList)
    {
        vtkActor* actor = pair.second;
        actor->VisibilityOff();
        actorsStatus[pair.first] = false;
    }
    actorsList[name] = cutActor;
    actorsStatus[name] = true;
}
/*******************************************************************
 *******************************************************************
 *******************************************************************
 **********************ColorMap*************************************
 *******************************************************************
 *******************************************************************
 *******************************************************************/
bool ColorMap::SetColorMapObject(std::string name, std::map<std::string, vtkActor *> &actorsList)
{
    if(actorsList.count(name)==0)
    {
        return false;
    }
    this->m_objName = name;
    this->m_selectedActor = actorsList[name];
    this->m_selectedStatus = true;
    return true;
}
bool ColorMap::SetSolidColor(std::map<std::string, vtkScalarBarActor *> &barsList, QColor RGBA)
{
    if(!m_selectedStatus) return false;
    //颜色转换
    int r=RGBA.red();
    int g=RGBA.green();
    int b=RGBA.blue();
    double normalizedR = r / 255.0;
    double normalizedG = g / 255.0;
    double normalizedB = b / 255.0;
    if(barsList.count(this->m_objName)!=0)
    {//如果之前执行过颜色映射，需要关闭颜色映射、设置bar不可见
        this->m_selectedActor->GetMapper()->ScalarVisibilityOff();
        barsList[this->m_objName]->VisibilityOff();
    }
    vtkProperty* property=this->m_selectedActor->GetProperty();
    property->SetColor(normalizedR,normalizedG,normalizedB);
    return true;
}
bool ColorMap::SetColorMapVariable(std::string selectedVar, std::map<std::string, vtkSmartPointer<vtkLookupTable>> &lutsList, std::map<std::string, vtkScalarBarActor *> &barsList, vtkRenderer *renderer)
{
    if(!m_selectedStatus) return false;
    //如果对于这个actor是第一次颜色映射，需要new个新的baractor和lookuptatle,并添加进renderer
    if(lutsList.count(this->m_objName)==0){
        vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
        auto barActor = vtkSmartPointer<vtkScalarBarActor>::New();
        lutsList[this->m_objName] = lut;
        barsList[this->m_objName] = barActor;
        renderer->AddActor2D(barActor);

    }
    auto lut = lutsList[this->m_objName];
    auto barActor = barsList[this->m_objName];
    vtkMapper* mapper= this->m_selectedActor->GetMapper();
    vtkDataSet* dataSet = vtkDataSet::SafeDownCast(mapper->GetInput());
    // 设置 Active Scalars
    int selectedId = dataSet->GetPointData()->SetActiveScalars(selectedVar.c_str());
    vtkDataArray* scalar = dataSet->GetPointData()->GetArray(selectedId);
    lut->SetTableRange(scalar->GetRange());
    lut->SetNumberOfColors(256);
    lut->Build();
    barActor->SetLookupTable(lut);
    mapper->SetScalarRange(scalar->GetRange());
    mapper->SetLookupTable(lut);

    mapper->ScalarVisibilityOn();
    barActor->VisibilityOn();
    return true;
}
/***************************************************************************
 ***************************************************************************
 ***************************************************************************
 **********************Contour的实现*****************************************
 ***************************************************************************
 ***************************************************************************
 ***************************************************************************/
void Contour::Initialize(std::string contourName, std::string derivedName, std::map<std::string, vtkActor *> &actorsList, vtkRenderer *renderer)
{
    // 初始化,新建contourfiler、polydatamapper、actor、renderer，但没设置actorsStatus
    this->m_contourFilter = vtkSmartPointer<vtkContourFilter>::New(); //新建一个filter

    vtkActor* derivedActor = actorsList[derivedName];
    this->m_data = vtkDataSet::SafeDownCast(derivedActor->GetMapper()->GetInput());//filter的输入
    this->m_contourFilter->SetInputData(this->m_data);
    this->m_contourFilter->Update();
    //filter连接mapper、actor
    vtkSmartPointer<vtkPolyDataMapper> contourMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    contourMapper->SetInputConnection(this->m_contourFilter->GetOutputPort());
    contourMapper->ScalarVisibilityOff();
    vtkSmartPointer<vtkActor> contourActor = vtkSmartPointer<vtkActor>::New();
    contourActor->SetMapper(contourMapper);
    actorsList[contourName] = contourActor;
    renderer->AddActor(contourActor);
}
double* Contour::SetActiveProperty(std::string propertyName)
{
    // 设置activescalar
    this->propertyStatus=true;
    int scalarId = this->m_data->GetPointData()->SetActiveScalars(propertyName.c_str());
    this->m_contourFilter->Update();
    return this->m_data->GetPointData()->GetArray(scalarId)->GetRange();
}
int Contour::AddEntry(double value)
{
    if(!propertyStatus) return -1;
    this->m_contourFilter->SetValue(this->m_valueNum,value);
    this->m_valueNum++;//指向下一个空的entry位置
    this->m_contourFilter->Update();
    return this->m_valueNum-1;//id从0开始
}
bool Contour::EditEntry(int entryId,double value)
{
    if(entryId < 0 || entryId >= this->m_valueNum){
        return false;
    }
    this->m_contourFilter->SetValue(entryId,value);
    this->m_contourFilter->Update();
    return true;
}
bool Contour::RemoveEntry(int entryId)
{
    if(entryId < 0 || entryId >= this->m_valueNum){
        return false;
    }
    std::vector<double> contourValues;
    for (int i = 0; i < this->m_valueNum; ++i)
    {
        if (i != entryId) // 跳过要删除的等值
        {
            contourValues.push_back(this->m_contourFilter->GetValue(i));
        }
    }
    this->m_valueNum--;
    this->m_contourFilter->SetNumberOfContours(this->m_valueNum);
    for (size_t i = 0; i < this->m_valueNum; ++i)
    {
        this->m_contourFilter->SetValue(static_cast<int>(i), contourValues[i]);
    }
    contourValues.clear();
    this->m_contourFilter->Update();
    return true;
}
/***************************************************************************
 ***************************************************************************
 ***************************************************************************
 **********************Glyph的实现*****************************************
 ***************************************************************************
 ***************************************************************************
 ***************************************************************************/
void Glyph::Initialize(std::string glyphName,std::string derivedName,std::map<std::string,vtkActor*>& actorsList,vtkRenderer* renderer)
{
     // 初始化,新建contourfiler、polydatamapper、actor、renderer，但没设置actorsStatus
    this->m_arrowSource = vtkSmartPointer<vtkArrowSource>::New();
    this->m_maskPoints = vtkSmartPointer<vtkMaskPoints>::New();
    this->m_glyphFilter = vtkSmartPointer<vtkGlyph3D>::New();

    vtkActor* derivedActor = actorsList[derivedName];
    this->m_data = vtkDataSet::SafeDownCast(derivedActor->GetMapper()->GetInput());//maskpoints的输入
    this->m_maskPoints->SetInputData(this->m_data);
    this->m_maskPoints->SetOnRatio(1000);
    this->m_maskPoints->RandomModeOn(); // 随机选择点
    //source设置（这些是默认
    this->m_arrowSource->SetTipLength(0.1);   // 设置箭头尖的长度
    this->m_arrowSource->SetTipRadius(0.01);  // 设置箭头尖的半径
    this->m_arrowSource->SetShaftRadius(0.001); // 设置箭头杆的半径
    //source和采样点接入filter
    this->m_glyphFilter->SetSourceConnection(this->m_arrowSource->GetOutputPort());
    this->m_glyphFilter->SetInputConnection(this->m_maskPoints->GetOutputPort());
    this->m_glyphFilter->SetVectorModeToUseVector();
    this->m_glyphFilter->SetScaleModeToScaleByVector();
    this->m_glyphFilter->SetScaleFactor(0.0001); // 调整箭头大小,原速度适配factor=0.00001
    //filter连接mapper、actor
    vtkSmartPointer<vtkPolyDataMapper> glyphMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    glyphMapper->SetInputConnection(this->m_glyphFilter->GetOutputPort());
    vtkSmartPointer<vtkActor> glyphActor = vtkSmartPointer<vtkActor>::New();
    glyphActor->SetMapper(glyphMapper);
    actorsList[glyphName] = glyphActor;
    renderer->AddActor(glyphActor);
}
void Glyph::SetGlyphVector(std::string vectorName)
{
    this->m_data->GetPointData()->SetActiveVectors(vectorName.c_str());
    this->m_data->Modified();
}
void Glyph::SetGlyphSourceTipLength(double tipLength)
{
    this->m_arrowSource->SetTipLength(tipLength);
    this->m_arrowSource->Modified();
    this->m_arrowSource->Update();
}
void Glyph::SetGlyphSourceTipRadius(double tipRadius)
{
    this->m_arrowSource->SetTipRadius(tipRadius);
    this->m_arrowSource->Modified();
    this->m_arrowSource->Update();
}
void Glyph::SetGlyphSourceShaftRadius(double shaftRadius)
{
    this->m_arrowSource->SetShaftRadius(shaftRadius);
    this->m_arrowSource->Modified();
    this->m_arrowSource->Update();
}
void Glyph::SetGlyphSourceScaleFactor(double scaleFactor)
{
    this->m_glyphFilter->SetScaleFactor(scaleFactor);
    this->m_glyphFilter->Modified();
    this->m_glyphFilter->Update();
}
void Glyph::SetGlyphPointsNumber(int pointsNumber)
{
    int n = this ->m_data->GetNumberOfPoints();
    int ratio = n/pointsNumber;
    this->m_maskPoints->SetOnRatio(ratio);
    this->m_maskPoints->Modified();
    this->m_arrowSource->Update();
}
void TecplotReader::pointsReader(int pointId, const std::string& line, int varNum, std::vector<vtkSmartPointer<vtkFloatArray>>& zoneData, vtkPoints* thePoints) {
    std::istringstream iss(line);
    std::string theValue;
    float x, y, z, tmp;
    for (int i = 0; i < varNum; i++)
    {
        iss >> theValue;
        tmp = stof(theValue);
        //std::cout << tmp << " ";
        zoneData[i]->SetValue(pointId, tmp);
        switch (i)
        {
        case 0:
            x = tmp;
            break;
        case 1:
            y = tmp;
            break;
        case 2:
            z = tmp;
            break;
        default:
            break;
        }
    }
    thePoints->InsertNextPoint(x, y, z);
}

void TecplotReader::cellsReader(const std::string& cellType, const std::string& line, vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid) {
    std::istringstream iss(line);
    std::string tmp;
    std::vector<int> realcell;
    auto ids = vtkSmartPointer<vtkIdList>::New();
    int cur = 0;
    while (iss >> tmp)
    {
        int theID = stoi(tmp) - 1;

        if (realcell.empty())
        {
            realcell.push_back(theID);
            ids->InsertNextId(theID);
        }
        else if (theID != realcell[cur]) {
            realcell.push_back(theID);
            cur++;
            ids->InsertNextId(theID);
        }
        //std::cout << "theID : " << theID << std::endl;
    }
    int len = realcell.size();
    switch (len)
    {
    case 3:
        unstructuredGrid->InsertNextCell(VTK_TRIANGLE, ids);
        break;
    case 4:
        if (cellType == "FEQUADRILATETAL")
            unstructuredGrid->InsertNextCell(VTK_QUAD, ids);
        else unstructuredGrid->InsertNextCell(VTK_TETRA, ids);
        break;
    case 5:
        unstructuredGrid->InsertNextCell(VTK_PYRAMID, ids);
        break;
    case 6:
        unstructuredGrid->InsertNextCell(VTK_WEDGE, ids);
        break;
    case 8:
        unstructuredGrid->InsertNextCell(VTK_HEXAHEDRON, ids);
        break;
    default:
        break;
    }
}

/***************************************************************************************
 *****************************数据读入tecplotreader具体实现********************************
 ***************************************************************************************/
vtkMultiBlockDataSet* TecplotReader::ReadTecplotData(const std::string &fileName) {
    //clock_t start_time = clock();

    std::ifstream file(fileName);
    std::string line;
    std::string currentToken;

    std::string title;
    int solutiontime = 0;
    int zoneNum = 0;
    int nodeNum; //the number of points

    std::vector<std::string> varName;

    std::vector<std::string> zoneTitle;
    std::vector<int> zoneCellNum;
    std::vector<std::string> zoneCellType;

    auto thePoints = vtkSmartPointer<vtkPoints>::New();
    std::vector<vtkSmartPointer<vtkFloatArray>> zoneData;
    //auto pointsData = vtkSmartPointer<vtkPoints>::New();
    vtkMultiBlockDataSet* multiBlock = vtkMultiBlockDataSet::New();
    auto sharedPointData = vtkSmartPointer<vtkPointData>::New();

    bool pointsOK = false;
    while (std::getline(file, line))
    {
        if (line.empty()) continue;
        if (!pointsOK)
        {
            if (line.find("TITLE") != std::string::npos)
            {
                int pos1 = line.find("\"");
                int pos2 = line.find("\"", pos1 + 1);
                title = line.substr(pos1 + 1, pos2 - pos1 - 1);
                //std::cout << title << std::endl;
                continue;
            }

            if (line.find("VARIABLES") != std::string::npos)
            {
                size_t pos1 = line.find("\"");
                size_t pos2 = line.find("\"", pos1 + 1);
                std::string tempVarName;
                while (pos2 != std::string::npos) {
                    tempVarName = line.substr(pos1 + 1, pos2 - pos1 - 1);
                    //std::cout << tempVarName << std::endl;
                    varName.push_back(tempVarName);
                    if (pos2 == line.size() - 1) break;
                    pos1 = line.find("\"", pos2 + 1);
                    pos2 = line.find("\"", pos1 + 1);
                }
                continue;
            }

            if (line.find("solutiontime") != std::string::npos)
            {
                std::istringstream iss(line);
                while (iss >> currentToken) {
                    if (isdigit(currentToken[0]))
                    {
                        solutiontime = std::stoi(currentToken);
                        break;
                    }
                }
                continue; //
            }

            if (line.find("ZONE") != std::string::npos)
            {
                zoneNum++;
                std::replace(line.begin(), line.end(), '=', ' ');
                std::replace(line.begin(), line.end(), ',', ' ');
                std::replace(line.begin(), line.end(), '"', ' ');
                std::istringstream iss(line);

                while (iss >> currentToken)
                {
                    if (currentToken == "T")
                    {
                        iss >> currentToken;
                        //std::cout <<"T=" <<currentToken << std::endl;
                        zoneTitle.push_back(currentToken);
                    }
                    else if (currentToken == "N")
                    {
                        iss >> nodeNum;
                        //std::cout <<"N="<< nodeNum << std::endl;
                    }
                    else if (currentToken == "E")
                    {
                        iss >> currentToken;
                        //std::cout <<"E="<< currentToken << std::endl;
                        zoneCellNum.push_back(std::stoi(currentToken));
                    }
                    else if (currentToken == "ZONETYPE")
                    {
                        iss >> currentToken;
                        zoneCellType.push_back(currentToken);
                        break;
                    }
                }
                continue;
            }

            int varNum = varName.size();

            for (int i = 0; i < varNum; i++) {
                vtkFloatArray* theArray = vtkFloatArray::New();
                theArray->SetNumberOfTuples(nodeNum);
                theArray->SetName(varName[i].c_str());
                zoneData.push_back(theArray);
                theArray->Delete();
            }
            pointsReader(0, line, varNum, zoneData, thePoints);


            for (int i = 1; i < nodeNum; i++)
            {
                std::getline(file, line);
                pointsReader(i, line, varNum, zoneData, thePoints);
            }
            pointsOK = true;
            continue;
        }

        if (line.find("ZONE") != std::string::npos)
        {
            zoneNum++;

            std::replace(line.begin(), line.end(), '=', ' ');
            std::replace(line.begin(), line.end(), '"', ' ');
            std::replace(line.begin(), line.end(), ',', ' ');
            std::istringstream iss(line);
            //std::cout << line << std::endl;
            while (iss >> currentToken)
            {
                if (currentToken == "T")
                {
                    iss >> currentToken;
                    //std::cout << currentToken << std::endl;
                    zoneTitle.push_back(currentToken);
                }
                else if (currentToken == "E")
                {
                    iss >> currentToken;
                    //std::cout << currentToken << std::endl;
                    zoneCellNum.push_back(std::stoi(currentToken));
                }
                else if (currentToken == "ZONETYPE")
                {
                    iss >> currentToken;
                    zoneCellType.push_back(currentToken);
                    break;
                }
            }
            continue;
        }

        auto ug = vtkSmartPointer<vtkUnstructuredGrid>::New();
        ug->SetPoints(thePoints);

        if (zoneNum == 1) {
            bool hasVelocity = false;
            for (auto& data : zoneData) {
                if(strcmp(data->GetName(), "vel") == 0 || strcmp(data->GetName(), "velocity") == 0)
                    hasVelocity = true;
                ug->GetPointData()->AddArray(data);
            }
            if(!hasVelocity)
            {
                auto calculator=vtkSmartPointer<vtkArrayCalculator>::New();
                //如果没有velocity属性，就进行添加
                calculator->SetInputData(ug);
                calculator->AddScalarArrayName("u");
                calculator->AddScalarArrayName("v");
                calculator->AddScalarArrayName("w");
                calculator->SetResultArrayName("velocity");
                calculator->SetFunction("u*iHat + v*jHat + w*kHat");
                calculator->Update();
                vtkDataArray* velocityArray = calculator->GetUnstructuredGridOutput()->GetPointData()->GetArray("velocity");
                ug->GetPointData()->AddArray(velocityArray);
            }
            sharedPointData = ug->GetPointData();
        }
        else {
            ug->GetPointData()->ShallowCopy(sharedPointData);
        }

        int zoneId = zoneNum - 1;
        //std::cout << "zoneId = " << zoneId << std::endl;
        cellsReader(zoneCellType[zoneId], line, ug);
        for (int i = 1; i < zoneCellNum[zoneId]; i++)
        {
            std::getline(file, line);
            cellsReader(zoneCellType[zoneId], line, ug);
        }
        multiBlock->SetBlock(zoneId, ug);
        multiBlock->GetMetaData(zoneId)->Set(vtkCompositeDataSet::NAME(), zoneTitle[zoneId]);
    }

    //clock_t end_time = clock();
    //std::cout << "" << (end_time - start_time) / (double)CLOCKS_PER_SEC << "s" << std::endl;

    return multiBlock;
}

