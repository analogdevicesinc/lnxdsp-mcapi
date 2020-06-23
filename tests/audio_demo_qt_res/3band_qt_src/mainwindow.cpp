/*
 ** Copyright (c) 2019, Analog Devices, Inc.  All rights reserved.
*/
#include "mainwindow.h"
#include <QMessageBox>
#include <QHostAddress>
#include <stdint.h>

Main::Main(QWidget *parent)
{
    setWindowTitle("Main");
    port =9734;    
    QString ipAddress;
    if (ipAddress.isEmpty())
        ipAddress = QHostAddress(QHostAddress::LocalHost).toString();

    tcpSocket = new QTcpSocket(this);
    tcpSocket->connectToHost(QHostAddress(ipAddress), port);
    nextPage = new Nextpage(this);

    QFont bq;
    bq.setPointSize(20);//font size for B M T VOL

    bass = new QSlider(Qt::Vertical);
    bass->setStyleSheet("QSlider::groove:vertical{border:5px;height:250px;}"
                         "QSlider::sub-page:vertical{background:darkgray;}"
                         "QSlider::add-page:vertical{background:darkgray;}"
                         "QSlider::handle:vertical{background:black;height:30px;width:20px;border-radius:0px;margin:-3px 0px -3px 0px;}");
    bass->setMaximumWidth(70);
    s1= new QLabel(tr("B"));
    s1->setFont(bq);

    mid = new QSlider(Qt::Vertical);
    mid->setStyleSheet("QSlider::groove:vertical{border:5px;height:250px;}"
                       "QSlider::sub-page:vertical{background:darkgray;}"
                       "QSlider::add-page:vertical{background:darkgray;}"
                       "QSlider::handle:vertical{background:black;height:30px;width:60px;border-radius:0px;margin:-3px 0px -3px 0px;}");
    mid->setMaximumWidth(70);
    s2= new QLabel(tr("M"));
    s2->setFont(bq);

    treble = new QSlider(Qt::Vertical);
    treble->setStyleSheet("QSlider::groove:vertical{border:5px;height:250px;}"
                          "QSlider::sub-page:vertical{background:darkgray;}"
                          "QSlider::add-page:vertical{background:darkgray;}"
                          "QSlider::handle:vertical{background:black;height:30px;width:20px;border-radius:0px;margin:-3px 0px -3px 0px;}");

     treble->setMaximumWidth(70);
     s3= new QLabel(tr("T"));
     s3->setFont(bq);

     bass->setMaximum(9);
     mid->setMaximum(9);
     treble->setMaximum(9);

 //vol sider
    slider = new QSlider(Qt::Horizontal);
    slider->setStyleSheet("QSlider::groove:horizontal{border:0px;width:600;}"
                             "QSlider::sub-page:horizontal{background:darkgray;}"
                             "QSlider::add-page:horizontal{background:darkgray;}"
                             "QSlider::handle:horizontal{background:black;height:20;width:30px;border-radius:0px;margin:-3px 0px -3px 0px;}");
    slider ->setMaximum(9);
    slider->setMaximumHeight(60);
    s4=new QLabel(tr("VOL"));
    s4->setFont(bq);

//next button
    QPushButton *next = new QPushButton("N",this);
    QFont zt;
    zt.setPointSize(20);
    next->setFont(zt);
    next->setFixedSize(40,40);//button size

//connection between sliders and styles
     QObject::connect(nextPage->pop, SIGNAL(clicked()), this, SLOT(popaction()));
     QObject::connect(nextPage->Jaz, SIGNAL(clicked()), this, SLOT(midaction()));
     QObject::connect(nextPage->Rock, SIGNAL(clicked()),this, SLOT(trebleaction()));

//next button for the second page
     QObject::connect(next, SIGNAL(clicked()), this, SLOT(nextpage()));


//vol shared between 2 pages
     QObject::connect(slider, SIGNAL(valueChanged(int)), nextPage->slider, SLOT(setValue(int)));
     QObject::connect(nextPage->slider, SIGNAL(valueChanged(int)), slider, SLOT(setValue(int)));

//value change on any slider will trigger
     QObject::connect(slider, SIGNAL(valueChanged(int)), this, SLOT(sendmsg()));
     QObject::connect(bass, SIGNAL(valueChanged(int)), this, SLOT(sendmsg()));
     QObject::connect(mid, SIGNAL(valueChanged(int)), this, SLOT(sendmsg()));
     QObject::connect(treble, SIGNAL(valueChanged(int)), this, SLOT(sendmsg()));

//layout
     toplayout = new QGridLayout;
     layout1 = new QVBoxLayout;

     layout1->addWidget(bass);
     layout1->addWidget(s1);
     toplayout->addLayout(layout1,2,2,6,3);

     layout2 = new QVBoxLayout;
     layout2->addWidget(mid);
     layout2->addWidget(s2);
     toplayout->addLayout(layout2,2,5,6,3);

     layout3 = new QVBoxLayout;
     layout3->addWidget(treble);
     layout3->addWidget(s3);
     toplayout->addLayout(layout3,2,8,6,3);

     toplayout->addWidget(next,7,9,2,1);

     botlayout = new QHBoxLayout;
     botlayout->addWidget(s4);
     botlayout->addWidget(slider);
     toplayout->addLayout(botlayout,9,1,2,9);

     this->setLayout(toplayout);

}

Main::~Main()
{
}



void Main::sendmsg()
 {
    v=slider->value();
    b=bass->value();
    m=mid->value();
    t=treble->value();
    QString p1 = QString("%1").arg(v);
    QString p2= QString("%1").arg(b);
    QString p3 = QString("%1").arg(m);
    QString p4= QString("%1").arg(t);

    QString msg = "-v"+p1+"-b"+p2+"-m"+p3+"-t"+p4;
     int length=0;
     if(msg=="")
     {
       return;
     }
    if((length=tcpSocket->write(msg.toLatin1(),msg.length()))!=msg.length())
     {
       return;
     }
 }

void Main::readResult(int exitCode)
{
    if(exitCode == 0)
    {
       QTextCodec* gbkCodec = QTextCodec::codecForName("GBK");
       QString result = gbkCodec->toUnicode(p->readAll());
     }
}

void Main::popaction()//default values
{

    disconnect(bass, SIGNAL(valueChanged(int)), 0, 0);
    disconnect(mid, SIGNAL(valueChanged(int)), 0, 0);
    bass->setValue(4);
    mid ->setValue(5);
    treble->setValue(6);



}
void Main::midaction()
{
    disconnect(bass, SIGNAL(valueChanged(int)), 0, 0);
    disconnect(mid, SIGNAL(valueChanged(int)), 0, 0);
    bass->setValue(2);
    mid->setValue(4);
    treble->setValue(7);

}
void Main::trebleaction()
{
    disconnect(bass, SIGNAL(valueChanged(int)), 0, 0);
    disconnect(mid, SIGNAL(valueChanged(int)), 0, 0);
    bass->setValue(7);
    mid->setValue(4);
    treble->setValue(6);

}
void Main::nextpage()
{
    this->close();
    nextPage->setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    nextPage->showMaximized();//window maxed
    nextPage->exec();
    this->show();
}

//**************************Nextpage**********************************************************************************************************


Nextpage::Nextpage(QWidget *parent):QDialog(parent)
{

      QPushButton *back = new QPushButton("B",this);
      back->setFixedSize(40,40);

      QFont zt;
      zt.setPointSize(20);
      back->setFont(zt);

      QObject::connect(back, SIGNAL(clicked()), this, SLOT(action()));

      setWindowTitle("Nextpage");

      pop = new QPushButton("P",this);
      pop->setFixedSize(100,100);
      QFont P;
      P.setPointSize(40);
      pop->setFont(P);

      Jaz = new QPushButton("J",this);
      Jaz->setFixedSize(100,100);
      QFont J;
      J.setPointSize(40);
      Jaz->setFont(J);
      
      Rock = new QPushButton("R",this);
      Rock->setFixedSize(100,100);
      QFont R;
      R.setPointSize(40);
      Rock->setFont(R);

      slider = new QSlider(Qt::Horizontal);
      slider->setStyleSheet("QSlider::groove:horizontal{border:0px;width:600;}"
                            "QSlider::sub-page:horizontal{background:darkgray;}"
                            "QSlider::add-page:horizontal{background:darkgray;}"
                            "QSlider::handle:horizontal{background:black;height:20;width:30px;border-radius:0px;margin:-3px 0px -3px 0px;}");
      slider->setMaximumHeight(60);
       
      QFont sl;
      sl.setPointSize(20);
      s5=new QLabel(tr("VOL"));
      s5->setFont(sl);
      slider->setMaximum(9);

      toplayout =new QGridLayout;

      toplayout->addWidget(pop,3,2,3,3);
      toplayout->addWidget(Jaz,3,5,3,3);
      toplayout->addWidget(Rock,3,8,3,3);

      toplayout->addWidget(back,5,9,1,1);
     
      botlayout = new QHBoxLayout;
      botlayout->addWidget(s5);
      botlayout->addWidget(slider);
      toplayout->addLayout(botlayout,6,1,2,9);
                                  
      this->setLayout(toplayout);

}

Nextpage::~Nextpage()
{
}

void Nextpage::action()
{
    this->close();

}

