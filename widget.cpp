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
       m_tecplotWidget->SetFileName(R"(D:\Project\VTK_QT\data\Tur_Merge_Field_[2000].dat)");
       /****测试渲染框颜色***/
       //m_tecplotWidget->SetBackgroundColor(QColor(173, 216, 230));
       //QColor tmp=m_tecplotWidget->GetBackgroundColor();
       //cout<<tmp.redF()<<" "<<tmp.greenF()<<" "<<tmp.blueF()<<endl;

       /****test about actor***/
       //QStringList actorList = m_tecplotWidget->GetActorList();
       //qInfo() << actorList;
       //QStringList propList = m_tecplotWidget->GetPropertyList("FLUID");
       //qInfo()<< propList;
       //qInfo() << m_tecplotWidget->GetPropertyName("FLUID",5);

       //m_tecplotWidget->ActorVisibilityOff("FLUID");
       //m_tecplotWidget->SetColorMapOn("P1","X");
       //m_tecplotWidget->SetColorMapOn("P2","X");
       //m_tecplotWidget->SetColorMapOff("P2");
       // m_tecplotWidget->ActorVisibilityOff("P1");
       //m_tecplotWidget->ActorVisibilityOn("P1");
       /***测试了等值线*

       //提取x=0.03的面，做速度矢量映射
       //m_tecplotWidget->AddContour("FLUID");
       //m_tecplotWidget->SetContouredBy("Contour1","X");
       //m_tecplotWidget->AddEntry("Contour1",0.03);
       //m_tecplotWidget->SetColorMapOn("Contour1","Y");
       //propList=m_tecplotWidget->GetPropertyList("Contour1");
       //qInfo()<<"add entry:get_contour1propertylist:";
       //qInfo()<<propList;
       m_tecplotWidget->ActorVisibilityOff("FLUID");
       m_tecplotWidget->ActorVisibilityOff("OUTLET");
       m_tecplotWidget->ActorVisibilityOff("P1");
       m_tecplotWidget->ActorVisibilityOff("P2");
       m_tecplotWidget->ActorVisibilityOff("INLET");
       m_tecplotWidget->ActorVisibilityOff("HUB");
       m_tecplotWidget->ActorVisibilityOff("BLADE");
       m_tecplotWidget->ActorVisibilityOff("SHR");
       m_tecplotWidget->ActorVisibilityOff("Contour1");
       m_tecplotWidget->ActorVisibilityOff("Slice1");
       //m_tecplotWidget->AddGlyph("Contour1");
       //m_tecplotWidget->SetGlyphVector("Glyph1","velocity");
       //m_tecplotWidget->SetGlyphPointsNumber("Glyph1",10);
       //m_tecplotWidget->SetGlyphSourceScaleFactor("Glyph1",0.1);
       qInfo()<<m_tecplotWidget->AddSliceWidget("FLUID");
       m_tecplotWidget->Slice("Slice1");
       qInfo()<<m_tecplotWidget->AddContour("Slice1");
       m_tecplotWidget->ActorVisibilityOff("Slice1");
       double* bounds=m_tecplotWidget->SetContouredBy("Contour1","T");
       QStringList contour1list = this->m_tecplotWidget->GetPropertyList("Contour1");
       qInfo()<< contour1list ;
       qInfo()<<bounds[0]<<bounds[1];  //289.497 341.827
       qInfo()<<m_tecplotWidget->AddEntry("Contour1",300);
       qInfo()<<m_tecplotWidget->AddEntry("Contour1",320);
       qInfo()<<m_tecplotWidget->AddEntry("Contour1",310);
       qInfo()<<m_tecplotWidget->EditEntry("Contour1",1,340);
       qInfo() <<m_tecplotWidget->RemoveEntry("Contour1",1);
       m_tecplotWidget->ActorVisibilityOn("Contour1");
       m_tecplotWidget->SetColorMapOn("Contour1","T");*/


       /***测试涡结构**
       this->m_tecplotWidget->CalculateQCriterion("FLUID");
       //propList = m_tecplotWidget->GetPropertyList("FLUID");
       //qInfo()<<propList;
       qInfo()<<m_tecplotWidget->AddContour("FLUID");
       double* bound = m_tecplotWidget->SetContouredBy("Contour1","QCriterion");
       m_tecplotWidget->SetColorMapOn("Contour1","QCriterion");
       qInfo()<<bound[0]<<bound[1];
       m_tecplotWidget->ActorVisibilityOff("FLUID");
       m_tecplotWidget->ActorVisibilityOff("OUTLET");
       m_tecplotWidget->ActorVisibilityOff("P1");
       m_tecplotWidget->ActorVisibilityOff("P2");
       m_tecplotWidget->ActorVisibilityOff("INLET");
       m_tecplotWidget->ActorVisibilityOff("HUB");
       m_tecplotWidget->ActorVisibilityOff("BLADE");
       m_tecplotWidget->ActorVisibilityOff("SHR");*/

       /***测试矢量图***
       qInfo()<<m_tecplotWidget->AddGlyph("FLUID");

       m_tecplotWidget->ActorVisibilityOff("P1");
       m_tecplotWidget->ActorVisibilityOff("P2");
       m_tecplotWidget->ActorVisibilityOff("INLET");
       m_tecplotWidget->ActorVisibilityOff("HUB");
       m_tecplotWidget->ActorVisibilityOff("BLADE");
       m_tecplotWidget->ActorVisibilityOff("SHR");
       m_tecplotWidget->ActorVisibilityOff("OUTLET");
       m_tecplotWidget->SetGlyphActiveVector("Glyph1","velocity");
       m_tecplotWidget->SetGlyphSourceTipRadius("Glyph1",0.01);
       m_tecplotWidget->SetGlyphPointsNumber("Glyph1",100);
       m_tecplotWidget->SetGlyphSourceScaleFactor("Glyph1",0.05);
       m_tecplotWidget->ActorVisibilityOn("FLUID");*/


       /***测试流场流线***/
       //z-buffter冲突问题
       m_tecplotWidget->AddContour("FLUID");
       m_tecplotWidget->SetContouredBy("Contour1","X");
       m_tecplotWidget->AddEntry("Contour1",0.03);
       m_tecplotWidget->ActorVisibilityOff("FLUID");
       m_tecplotWidget->ActorVisibilityOff("P1");
       m_tecplotWidget->ActorVisibilityOff("P2");
       m_tecplotWidget->ActorVisibilityOff("INLET");
       m_tecplotWidget->ActorVisibilityOff("HUB");
       m_tecplotWidget->ActorVisibilityOff("BLADE");
       m_tecplotWidget->ActorVisibilityOff("SHR");
       m_tecplotWidget->ActorVisibilityOff("OUTLET");
       m_tecplotWidget->ActorVisibilityOff("Contour1");
       //m_tecplotWidget->SetColorMapOn("Contour1","T");
       //m_tecplotWidget->SetSolidColor("Contour1",QColor(1,1,1));
       qInfo()<<m_tecplotWidget->AddStreamTracer("Contour1");
       qInfo()<<"contour1 propertylist:"<<m_tecplotWidget->GetPropertyList("Contour1");
       qInfo()<<"streamtracer propertylist "<<m_tecplotWidget->GetPropertyList("StreamTracer1");
       //qInfo()<<m_tecplotWidget->GetPropertyList("StreamTracer1");
       //m_tecplotWidget->SetStreamTracerRatio("StreamTracer1",10,100000);
       //m_tecplotWidget->SetStreamTracerDiretion("StreamTracer1",0);
       //m_tecplotWidget->SetStreamTracerMaximumPropagation("StreamTracer1",10);
       //m_tecplotWidget->SetStreamTracerIntegrationStepUnit("StreamTracer1",2);
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

