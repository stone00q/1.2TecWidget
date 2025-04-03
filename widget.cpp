#include "widget.h"
#include "ui_widget.h"

#include <qgridlayout.h>
#include <qpushbutton.h>
Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    //创建自定义widget实例
       m_tecplotWidget = new TecplotWidget(this);
       //在当前widget上布局
       QVBoxLayout* layout = new QVBoxLayout(this);
       layout->addWidget(m_tecplotWidget);
//       m_tecplotWidget->SetFileName(R"(D:\Project\VTK_QT\data\ROTOR67-flow_sa_[8000]_1.dat)");//Actor list: ("FLUID", "HUB", "INLET", "OUTLET", "PA", "PB", "PS", "SHROUD", "SS", "TIP")
//       /***测试是s1面***/
//       qInfo()<<m_tecplotWidget->GetPropertyList("FLUID");
//       m_tecplotWidget->ActorVisibilityOff("HUB");
//       m_tecplotWidget->ActorVisibilityOff("INLET");
//       m_tecplotWidget->ActorVisibilityOff("OUTLET");
//       m_tecplotWidget->ActorVisibilityOff("PA");
//       m_tecplotWidget->ActorVisibilityOff("PB");
//       m_tecplotWidget->ActorVisibilityOff("PS");
//       m_tecplotWidget->ActorVisibilityOff("SHROUD");
//       m_tecplotWidget->ActorVisibilityOff("SS");
//       m_tecplotWidget->ActorVisibilityOff("TIP");
//       m_tecplotWidget->SetSolidOpacity("FLUID",0.5);
//       qInfo()<<m_tecplotWidget->ExtracteS1("PA","FLUID");
//       qInfo()<<m_tecplotWidget->GetPropertyList("S1RelativeR=50%");
//       m_tecplotWidget->SetColorMapOn("S1RelativeR=50%","p");
//       m_tecplotWidget->SetColorLineOn("S1RelativeR=50%");

       /*****测试contour抽取多个等值面后进行颜色映射*******/
//       m_tecplotWidget->SetFileName(R"(D:\Project\VTK_QT\data\Tur_Merge_Field_[2000].dat)");
//       qInfo()<<m_tecplotWidget->GetPropertyList("FLUID");
//       m_tecplotWidget->ActorVisibilityOff("FLUID");
//       m_tecplotWidget->ActorVisibilityOff("OUTLET");
//       m_tecplotWidget->ActorVisibilityOff("P1");
//       m_tecplotWidget->ActorVisibilityOff("P2");
//       m_tecplotWidget->ActorVisibilityOff("INLET");
//       m_tecplotWidget->ActorVisibilityOff("HUB");
//       m_tecplotWidget->ActorVisibilityOff("BLADE");
//       m_tecplotWidget->ActorVisibilityOff("SHR");
//       m_tecplotWidget->CalculateQCriterion("FLUID");
//       m_tecplotWidget->AddContour("FLUID");
//       m_tecplotWidget->SetContouredBy("Contour1","X");
//       m_tecplotWidget->AddEntry("Contour1",0);
//      m_tecplotWidget->AddEntry("Contour1",0.05);
       //qInfo()<<m_tecplotWidget->GetPropertyList("Contour1");
       //m_tecplotWidget->UpdateAllPropertyForContour("Contour1");
       //qInfo()<<m_tecplotWidget->GetPropertyList("Contour1");
       //m_tecplotWidget->SetColorMapOn("Contour1","Y");
       //m_tecplotWidget->AddEntry("Contour1",-0.05);
//       m_tecplotWidget->SetContouredBy("Contour1","QCriterion");
//       m_tecplotWidget->AddEntry("Contour1",100);
//       m_tecplotWidget->AddEntry("Contour1",10000);
//       m_tecplotWidget->AddEntry("Contour1",300000);
//       m_tecplotWidget->SetColorMapOn("Contour1","QCriterion");
//       m_tecplotWidget->SetColorMapBounds("Contour1",150,20000);

//       /***测试多个actor颜色映射打开和关闭*****/
  m_tecplotWidget->SetFileName(R"(D:\Project\VTK_QT\data\Tur_Merge_Field_[2000].dat)");
       qInfo()<<m_tecplotWidget->GetPropertyList("FLUID");
              m_tecplotWidget->ActorVisibilityOff("FLUID");
              m_tecplotWidget->SetColorMapOn("OUTLET","p");
              m_tecplotWidget->SetColorMapOn("P1","T");
              m_tecplotWidget->SetColorMapOn("P2","rho");
              m_tecplotWidget->SetColorMapOn("INLET","res-E");
              //m_tecplotWidget->SetColorMapOff("P1");
              m_tecplotWidget->ActorVisibilityOff("HUB");
              m_tecplotWidget->ActorVisibilityOff("BLADE");
              m_tecplotWidget->ActorVisibilityOff("SHR");
              m_tecplotWidget->SetColorMapBounds("P1",300,600);



       //m_tecplotWidget->SetFileName(R"(D:\Project\VTK_QT\data\test_ctn_[10].dat)");
       // 获取 Block 数量和 Actor 列表
//       int blockNum = m_tecplotWidget->GetNumberOfBlock();
//       QStringList actorList = m_tecplotWidget->GetActorList();
//       qInfo() << "Number of blocks:" << blockNum;
//       qInfo() << "Actor list:" << actorList;
//       m_tecplotWidget->GetPropertyList("HUB");
//       m_tecplotWidget->GetPropertyBounds("P1","velocity");
//       m_tecplotWidget->GetPropertyBounds("P2","p");
//       m_tecplotWidget->GetPropertyBounds("P1","x");
//              m_tecplotWidget->ActorVisibilityOff("FLUID");
//              m_tecplotWidget->ActorVisibilityOff("OUTLET");
//              //m_tecplotWidget->ActorVisibilityOff("P1");
//              m_tecplotWidget->ActorVisibilityOff("P2");
//              m_tecplotWidget->ActorVisibilityOff("INLET");
//              m_tecplotWidget->ActorVisibilityOff("HUB");
//              m_tecplotWidget->ActorVisibilityOff("BLADE");
//              m_tecplotWidget->ActorVisibilityOff("SHR");
//              m_tecplotWidget->SetColorMapOn("P1","X");
//              qInfo()<<m_tecplotWidget->AddContour("FLUID");
//              m_tecplotWidget->SetContouredBy("Contour1","X");
//              m_tecplotWidget->AddEntry("Contour1",0);
//              m_tecplotWidget->AddEntry("Contour1",0.05);
//              qInfo()<<m_tecplotWidget->GetPropertyList("Contour1");
//             m_tecplotWidget->SetColorMapOn("Contour1","Z");
              //m_tecplotWidget->GetPropertyBounds("Contour1","X");///???代码有问题
       //m_tecplotWidget->SetFileName(R"(D:\Project\VTK_QT\data\f3.dat)");
       //test_ctn_[10]
       //m_tecplotWidget->SetFileName(R"(D:\Project\VTK_QT\data\test_ctn_[20].dat)");
//       /****测试渲染框颜色***/
//       //m_tecplotWidget->SetBackgroundColor(QColor(173, 216, 230));
//       //QColor tmp=m_tecplotWidget->GetBackgroundColor();
//       //cout<<tmp.redF()<<" "<<tmp.greenF()<<" "<<tmp.blueF()<<endl;

//       /****test about actor***/
//       //QStringList actorList = m_tecplotWidget->GetActorList();
//       //qInfo() << actorList;
//       QStringList propList = m_tecplotWidget->GetPropertyList("FLUID");
//       qInfo()<< propList;
//       //qInfo() << m_tecplotWidget->GetPropertyName("FLUID",5);

//       m_tecplotWidget->ActorVisibilityOff("FLUID");
//       //m_tecplotWidget->SetColorMapOn("P1","X");
//       //m_tecplotWidget->SetColorMapOn("P2","X");
//       //m_tecplotWidget->SetColorMapOff("P2");
//       // m_tecplotWidget->ActorVisibilityOff("P1");
//       //m_tecplotWidget->ActorVisibilityOn("P1");
//       /***测试了等值线**/


       /**contour抽取等值面之后颜色映射有问题，但是还没有修改**/


       /**测试x截面：成功**/
//       qInfo()<<m_tecplotWidget->AddSliceWidget("FLUID");
//       m_tecplotWidget->SliceByXPlane("Slice1",0.03);
//       m_tecplotWidget->HideSliceWidget("Slice1");
//       //m_tecplotWidget->SliceByZPlane("Slice1",0);
//       m_tecplotWidget->SetColorMapOn("Slice1","Y");
//       m_tecplotWidget->SetColorMapOff("Slice1");
//       m_tecplotWidget->ShowSliceWidget("Slice1");
//       m_tecplotWidget->ActorVisibilityOff("Slice1");//成功和交互器一起关掉了
//       m_tecplotWidget->ActorVisibilityOn("Slice1");//成功和交互器一起打开
       //m_tecplotWidget->EnableSliceInteraction("Slice1");//重新启用交互器
//       m_tecplotWidget->SetNumberOfColor("Slice1",25);
//       m_tecplotWidget->SetColorLineOn("Slice1");
//       m_tecplotWidget->ActorVisibilityOff("FLUID");
//       m_tecplotWidget->ActorVisibilityOff("OUTLET");
//       //m_tecplotWidget->ActorVisibilityOff("P1");
//       m_tecplotWidget->ActorVisibilityOff("P2");
//       m_tecplotWidget->ActorVisibilityOff("INLET");
//       m_tecplotWidget->ActorVisibilityOff("HUB");
//       m_tecplotWidget->ActorVisibilityOff("BLADE");
//       m_tecplotWidget->ActorVisibilityOff("SHR");
       /***多个actor颜色映射***/
//        m_tecplotWidget->ActorVisibilityOff("FLUID");
//              m_tecplotWidget->ActorVisibilityOff("OUTLET");
//              m_tecplotWidget->ActorVisibilityOff("INTLET");
//              m_tecplotWidget->ActorVisibilityOff("P1-R");
//              m_tecplotWidget->ActorVisibilityOff("P2-R");
//              m_tecplotWidget->ActorVisibilityOff("INLET-R");
//              m_tecplotWidget->ActorVisibilityOff("HUB-R");
//              m_tecplotWidget->ActorVisibilityOff("SHR-R");
//              m_tecplotWidget->ActorVisibilityOff("P1-S");
//              m_tecplotWidget->ActorVisibilityOff("P2-S");
//              m_tecplotWidget->ActorVisibilityOff("INLET-S");
//              m_tecplotWidget->ActorVisibilityOff("HUB-S");
//              m_tecplotWidget->ActorVisibilityOff("SHR-S");
//              m_tecplotWidget->SetColorMapOn("OUTLET","rho");
//              m_tecplotWidget->SetColorMapOn("P1-R","p");
//              m_tecplotWidget->SetColorMapOn("P2-R","T");
//              m_tecplotWidget->SetColorMapOn("FLUID","Rg");




       // 设置等值面参数和颜色映射
       // 添加多个等值面Actor
//       m_tecplotWidget->AddContour("FLUID"); // 生成Contour1
//       m_tecplotWidget->AddContour("FLUID"); // 生成Contour2
//       m_tecplotWidget->AddContour("FLUID"); // 生成Contour3
//          for(int i = 0; i < 3; ++i) {
//              QString contourName = QString("Contour%1").arg(i+1);
//              m_tecplotWidget->SetContouredBy(contourName, "X");
//              m_tecplotWidget->AddEntry(contourName, -0.03 + i*0.03); // 设置不同Z值位置
//              m_tecplotWidget->SetColorMapOn(contourName, "X");
//          }
//       /****提取x=0.03的面，做速度矢量映射****/
//       m_tecplotWidget->AddContour("FLUID");
//       m_tecplotWidget->SetContouredBy("Contour1","Z");
//       m_tecplotWidget->AddEntry("Contour1",-0.03);
//       m_tecplotWidget->SetColorMapOn("Contour1","");
       //propList=m_tecplotWidget->GetPropertyList("Contour1");
       //qInfo()<<"add entry:get_contour1propertylist:";
//       //qInfo()<<propList;
//       m_tecplotWidget->ActorVisibilityOff("FLUID");
//       m_tecplotWidget->ActorVisibilityOn("OUTLET");
//       m_tecplotWidget->ActorVisibilityOn("P1");
//       m_tecplotWidget->ActorVisibilityOn("P2");
//       m_tecplotWidget->SetColorMapOn("OUTLET", "p");
//       m_tecplotWidget->SetColorMapOn("P1", "rho");
//       m_tecplotWidget->SetColorMapOn("P2", "thermal-cond");
//       m_tecplotWidget->ActorVisibilityOff("INLET");
//       m_tecplotWidget->ActorVisibilityOff("HUB");//??
//       m_tecplotWidget->ActorVisibilityOff("BLADE");//?
//       m_tecplotWidget->ActorVisibilityOff("SHR");//?
//       m_tecplotWidget->ActorVisibilityOff("Contour1");
//       m_tecplotWidget->ActorVisibilityOff("Slice1");
//       m_tecplotWidget->AddGlyph("Contour1");
//       qInfo()<<m_tecplotWidget->GetPropertyList("HUB");

//       m_tecplotWidget->SetGlyphActiveVector("Glyph1","velocity");
//       m_tecplotWidget->SetSolidColor("Glyph1",QColor(1,0,0));
//       m_tecplotWidget->SetGlyphPointsNumber("Glyph1",1000);
//       m_tecplotWidget->SetGlyphSourceScaleFactor("Glyph1",0.5);
//       qInfo()<<m_tecplotWidget->AddSliceWidget("FLUID");
//       m_tecplotWidget->Slice("Slice1");
//       qInfo()<<m_tecplotWidget->AddContour("Slice1");
//       m_tecplotWidget->ActorVisibilityOff("Slice1");
//       double* bounds=m_tecplotWidget->SetContouredBy("Contour1","T");
//       QStringList contour1list = this->m_tecplotWidget->GetPropertyList("Contour1");
//       qInfo()<< contour1list ;
//       qInfo()<<bounds[0]<<bounds[1];  //289.497 341.827
//       qInfo()<<m_tecplotWidget->AddEntry("Contour1",300);
//       qInfo()<<m_tecplotWidget->AddEntry("Contour1",320);
//       qInfo()<<m_tecplotWidget->AddEntry("Contour1",310);
//       qInfo()<<m_tecplotWidget->EditEntry("Contour1",1,340);
//       qInfo() <<m_tecplotWidget->RemoveEntry("Contour1",1);
//       m_tecplotWidget->ActorVisibilityOn("Contour1");
//       m_tecplotWidget->SetColorMapOn("Contour1","T");


       /***测试contour***/
//              m_tecplotWidget->ActorVisibilityOff("FLUID");
//              m_tecplotWidget->ActorVisibilityOff("OUTLET");
//              m_tecplotWidget->ActorVisibilityOff("P1");
//              m_tecplotWidget->ActorVisibilityOff("P2");
//              m_tecplotWidget->SetColorMapOn("OUTLET", "p");
//              m_tecplotWidget->SetColorMapOn("P1", "rho");
//              m_tecplotWidget->SetColorMapOn("P2", "thermal-cond");
//              m_tecplotWidget->ActorVisibilityOff("INLET");
//              m_tecplotWidget->ActorVisibilityOff("HUB");//??
//              m_tecplotWidget->ActorVisibilityOff("BLADE");//?
//              m_tecplotWidget->ActorVisibilityOff("SHR");//?
//              auto name1=m_tecplotWidget->AddContour("FLUID");
//              m_tecplotWidget->SetContouredBy(name1,"X");
//              auto entry1=m_tecplotWidget->AddEntry(name1,-0.03);
//              auto entry2=m_tecplotWidget->AddEntry(name1,0);
//              auto entry3=m_tecplotWidget->AddEntry(name1,0.05);
//              auto entry4=m_tecplotWidget->AddEntry(name1,0.057);
//              m_tecplotWidget->SetContouredBy(name1,"Y");
//       /***测试涡结构**/
//       this->m_tecplotWidget->CalculateQCriterion("FLUID");
////       //propList = m_tecplotWidget->GetPropertyList("FLUID");
////       //qInfo()<<propList;
//       qInfo()<<m_tecplotWidget->AddContour("FLUID");
//       double* bound = m_tecplotWidget->SetContouredBy("Contour1","QCriterion");


       /**测试截面，但是好像有问题？？？？-----颜色映射有问题***/
//       qInfo()<<m_tecplotWidget->AddEntry("Contour1",100);
//       qInfo()<<m_tecplotWidget->AddEntry("Contour1",10000000);
//       qInfo()<<m_tecplotWidget->AddEntry("Contour1",120000);
//       //m_tecplotWidget->SetColorMapOn("Contour1","QCriterion");
//       qInfo()<<m_tecplotWidget->GetPropertyList("Contour1");
//       //qInfo()<<bound[0]<<bound[1];
//       m_tecplotWidget->ActorVisibilityOff("FLUID");
//       m_tecplotWidget->SetColorMapOn("Contour1","X");
//       m_tecplotWidget->ActorVisibilityOff("OUTLET");
//       m_tecplotWidget->ActorVisibilityOff("P1");
//       m_tecplotWidget->ActorVisibilityOff("P2");
//       m_tecplotWidget->ActorVisibilityOff("INLET");
//       m_tecplotWidget->ActorVisibilityOff("HUB");
//       m_tecplotWidget->ActorVisibilityOff("BLADE");
//       m_tecplotWidget->ActorVisibilityOff("SHR");
//       //数据2test_ctn_[10].dat
//       m_tecplotWidget->ActorVisibilityOff("OUTLET");
//       m_tecplotWidget->ActorVisibilityOff("INTLET");
//       m_tecplotWidget->ActorVisibilityOff("P1-R");
//       m_tecplotWidget->ActorVisibilityOff("P2-R");
//       m_tecplotWidget->ActorVisibilityOff("INLET-R");
//       m_tecplotWidget->ActorVisibilityOff("HUB-R");
//       m_tecplotWidget->ActorVisibilityOff("SHR-R");
//       m_tecplotWidget->ActorVisibilityOff("P1-S");
//       m_tecplotWidget->ActorVisibilityOff("P2-S");
//       m_tecplotWidget->ActorVisibilityOff("INLET-S");
//       m_tecplotWidget->ActorVisibilityOff("HUB-S");
//       m_tecplotWidget->ActorVisibilityOff("SHR-S");

//       /***测试矢量图,矢量化的箭头不具有propertylist****/
//       qInfo()<<m_tecplotWidget->AddGlyph("HUB");
//       QColor redColor(Qt::red);
//       m_tecplotWidget->ActorVisibilityOff("P1");
//       m_tecplotWidget->ActorVisibilityOff("P2");
//       m_tecplotWidget->ActorVisibilityOff("INLET");
//       m_tecplotWidget->ActorVisibilityOff("HUB");
//       m_tecplotWidget->ActorVisibilityOff("BLADE");
//       m_tecplotWidget->ActorVisibilityOff("SHR");
//       m_tecplotWidget->ActorVisibilityOff("OUTLET");
//       m_tecplotWidget->ActorVisibilityOff("FLUID");
//       m_tecplotWidget->SetGlyphActiveVector("Glyph1","velocity");
//       m_tecplotWidget->SetGlyphSourceTipRadius("Glyph1",0.02);
//       m_tecplotWidget->SetGlyphSourceShaftRadius("Glyph1",0.005);
//       m_tecplotWidget->SetGlyphPointsNumber("Glyph1",200);
//       m_tecplotWidget->SetGlyphSourceScaleFactor("Glyph1",0.0001);
//       m_tecplotWidget->SetSolidColor("Glyph1",redColor);
//       m_tecplotWidget->ActorVisibilityOn("HUB");
//       m_tecplotWidget->GetPropertyBounds("HUB","velocity");
//       m_tecplotWidget->GetPropertyList("Glyph1");//矢量化图形不具有property

//       /***测试流场流线***/
//       //z-buffter冲突问题
//       m_tecplotWidget->AddContour("FLUID");
//       m_tecplotWidget->SetContouredBy("Contour1","X");
//       m_tecplotWidget->AddEntry("Contour1",0.03);
//       m_tecplotWidget->ActorVisibilityOff("FLUID");
//       m_tecplotWidget->ActorVisibilityOff("P1");
//       m_tecplotWidget->ActorVisibilityOff("P2");
//       m_tecplotWidget->ActorVisibilityOff("INLET");
//       m_tecplotWidget->ActorVisibilityOff("HUB");
//       m_tecplotWidget->ActorVisibilityOff("BLADE");
//       m_tecplotWidget->ActorVisibilityOff("SHR");
//       m_tecplotWidget->ActorVisibilityOff("OUTLET");
//       m_tecplotWidget->ActorVisibilityOff("Contour1");
//       //m_tecplotWidget->SetColorMapOn("Contour1","T");
//       //m_tecplotWidget->SetSolidColor("Contour1",QColor(1,1,1));
//       qInfo()<<m_tecplotWidget->AddStreamTracer("Contour1");
//       qInfo()<<"contour1 propertylist:"<<m_tecplotWidget->GetPropertyList("Contour1");
//       qInfo()<<"streamtracer propertylist "<<m_tecplotWidget->GetPropertyList("StreamTracer1");
//       //qInfo()<<m_tecplotWidget->GetPropertyList("StreamTracer1");
//       //m_tecplotWidget->SetStreamTracerRatio("StreamTracer1",10,100000);
//       //m_tecplotWidget->SetStreamTracerDiretion("StreamTracer1",0);
//       //m_tecplotWidget->SetStreamTracerMaximumPropagation("StreamTracer1",10);
//       //m_tecplotWidget->SetStreamTracerIntegrationStepUnit("StreamTracer1",2);

       /**测试S1平行曲面**/
       //m_tecplotWidget->testS1();
}

Widget::~Widget()
{
    delete ui;
}

void Widget::BackgroundButton_clicked()
{
    //渲染窗口为浅蓝色
    QColor color=QColor(173, 216, 230);
    m_tecplotWidget->SetBackgroundColor(color);
}
void Widget::SolidColorButton_clicked()
{
    //固体为绿色
    QColor green(0, 255, 0);
    //m_tecplotWidget->SetSolidColor(green);
}
/*
void Widget::XButton_clicked()
{
    //x坐标颜色映射
    //tecplotWidget->SetColorMapVariable("p");
    tecplotWidget->SetColorMappingFlag(true);
}*/
void Widget::basicButton1_clicked()
{
    QColor green(0, 255, 0);
    m_tecplotWidget->SetSolidColor("FLUID",green);
}
void Widget::basicButton2_clicked()
{
    m_tecplotWidget->SetColorMapOn("FLUID","p");

}
void Widget::slice1Button1_clicked()
{
    //m_tecplotWidget->SetColorMapObject("Slice1");
    QColor red(255, 0, 0);
    //m_tecplotWidget->SetSolidColor(red);
}
void Widget::slice1Button2_clicked()
{
    m_tecplotWidget->SetColorMapOn("Slice1","X");
}

void Widget::XButton_clicked()
{//需要先点击
    QString name = m_tecplotWidget->AddContour("basicActor");
    m_tecplotWidget->SetContouredBy(name,"QCriterion");
    cout<<name.toStdString()<<endl;
    int entryid = m_tecplotWidget->AddEntry(name,1000);

}
void Widget::YButton_clicked()
{
    QString name = m_tecplotWidget->AddContour("Contour1");
    m_tecplotWidget->SetContouredBy(name,"T");
    //cout<<name.toStdString()<<endl;
    m_tecplotWidget->AddEntry(name,350);
    m_tecplotWidget->AddEntry(name,300);

}
void Widget::Xcolor_clicked()
{
    m_tecplotWidget->SetColorMapOn("Contour1","p");
}
void Widget::GlyphButton_clicked()
{
    QString name = m_tecplotWidget->AddGlyph("Contour1");
    m_tecplotWidget->SetGlyphActiveVector(name,"velocity");
    m_tecplotWidget->SetGlyphPointsNumber(name,1000);

}
void Widget::QButton_clicked()
{
    m_tecplotWidget->CalculateQCriterion("basicActor");
}

