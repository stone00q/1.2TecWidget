#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QProcess>
#include <QDebug>
#include "TecplotWidget.h"
QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
private slots:
    void BackgroundButton_clicked();
    void SolidColorButton_clicked();
    //void XButton_clicked();


    void basicButton1_clicked();
    void basicButton2_clicked();
    void slice1Button1_clicked();
    void slice1Button2_clicked();

    void XButton_clicked();
    void YButton_clicked();
    void Xcolor_clicked();

    void GlyphButton_clicked();
    void QButton_clicked();

private:
    Ui::Widget *ui;
    TecplotWidget* m_tecplotWidget;
};
#endif // WIDGET_H
