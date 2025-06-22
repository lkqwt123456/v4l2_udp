#include "widget.h"
#include "ui_widget.h"
#include <QMessageBox>
#include <QTimer>
#include <QDebug>
#include <QPixmap>
#include <QPainter>
#include <QPen>
#include <QUdpSocket>


Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    server = new QUdpSocket(this);
    client = new QUdpSocket(this);

    server->bind(QHostAddress::Any,9999);
    connect(server, &QUdpSocket::readyRead, this, &Widget::read_video_data);


    disp_box_width = 320;
    disp_box_height = 240;
    ui->label->setGeometry(0,0,disp_box_width,disp_box_height);

    pix = new QPixmap(disp_box_width,disp_box_height);
    pix->fill(Qt::black);
    ui->label->setPixmap(*pix);

    cam_name = "/dev/video0";


    //video_fmt = "MJPEG";
    video_fmt = "YUYV";

    width = 320;
    hight = 240;

    cam_raw_buf_len =  width * hight * 4;
    cam_rgb_buf_len =  width * hight * 4;
    frame_len = 0;

    frameCount = 0;

    cam_raw_buf = (uchar *) malloc (cam_raw_buf_len);
    // if (cam_raw_buf  == ((void *) -1) )
    if (cam_raw_buf  == MAP_FAILED)
    {
        qDebug()  <<   "mem malloc failed ,program exit!!";
        exit(1);
    }

    cam_rgb_buf = (uchar *) malloc (cam_rgb_buf_len);

    if (cam_rgb_buf  == MAP_FAILED )
    {
        qDebug()  <<   "mem malloc failed ,program exit!!";
        exit(1);
    }


    image  = new QImage(cam_rgb_buf,width,hight,QImage::Format_RGB888);
    timer =  new QTimer(this);


    painter = new QPainter(this);

    curDateTime = QDateTime::currentDateTime();
    preDateTime = curDateTime;

    cam_vd  =  new  VideoDevice(cam_name);

    if ( cam_vd->open_device() < 0 )
    {
        QMessageBox::critical(this,"Device Open Error",QString("Open device %1  failed!").arg(cam_name));
        exit(1);
    }

    if (cam_vd->init_device(width,hight,video_fmt) < 0 )
    {
        QMessageBox::critical(this,"Device Init Error",QString("Open init  %1  failed!").arg(cam_name));
        cam_vd->close_device();
        exit(1);

    }

    if ( cam_vd->start_capturing()  < 0)
    {
        QMessageBox::critical(this,"Device Start Capture  Error",QString("Open device %1  failed!").arg(cam_name));
        cam_vd->close_device();

    }

    connect(timer,&QTimer::timeout,this,[=]{
        int ret=0;
        ret = cam_vd->get_frame((void **) &cam_raw_buf, &frame_len);
        if( ret < 0 )
        {
            qDebug() << "get _frame error!";
            return;
        }

        cam_vd->unget_frame();
        yuyv422_to_rgb888(cam_raw_buf,cam_rgb_buf,width,hight);
        send_video_data();

    });

    timer->start(1000/30);
}


Widget::~Widget()
{
   cam_vd->stop_capturing();
   cam_vd->uninit_device();
   cam_vd->close_device();
   qDebug() << "program closed";
    delete ui;
}


void Widget::yuyv422_to_rgb888(unsigned char *yuyvdata, unsigned char *rgbdata, int w, int h)
{
    //码流Y0 U0 Y1 V1 Y2 U2 Y3 V3 --》YUYV像素[Y0 U0 V1] [Y1 U0 V1] [Y2 U2 V3] [Y3 U2 V3]--》RGB像素
    int r1, g1, b1;
    int r2, g2, b2;
    int i;

    for(i=0; i<w*h/2; i++)
    {
        char data[4];
        memcpy(data, yuyvdata+i*4, 4);
        unsigned char Y0=data[0];
        unsigned char U0=data[1];
        unsigned char Y1=data[2];
        unsigned char V1=data[3];

        r1 = Y0+1.4075*(V1-128);
        if(r1>255) r1=255;
        if(r1<0)   r1=0;

        g1 =Y0- 0.3455 * (U0-128) - 0.7169*(V1-128);
        if(g1>255)  g1=255;
        if(g1<0)    g1=0;

        b1 = Y0 + 1.779 * (U0-128);
        if(b1>255)  b1=255;
        if(b1<0)    b1=0;

        r2 = Y1+1.4075*(V1-128);
        if(r2>255)  r2=255;
        if(r2<0)    r2=0;

        g2 = Y1- 0.3455 * (U0-128) - 0.7169*(V1-128);
        if(g2>255)    g2=255;
        if(g2<0)      g2=0;

        b2 = Y1 + 1.779 * (U0-128);
        if(b2>255)    b2=255;
        if(b2<0)      b2=0;

        rgbdata[i*6+0]=r1;
        rgbdata[i*6+1]=g1;
        rgbdata[i*6+2]=b1;
        rgbdata[i*6+3]=r2;
        rgbdata[i*6+4]=g2;
        rgbdata[i*6+5]=b2;
    }
}


void Widget::send_video_data(){
    QHostAddress srv_ip = QHostAddress("127.0.0.1");
    quint16 srv_port = 9999;

    // 发送视频数据到UDP服务器
    qDebug()<<"begin";
    client->writeDatagram((const char*)cam_raw_buf, cam_rgb_buf_len, srv_ip, srv_port);
    qDebug()<<"send success!";
    read_video_data();
}


void Widget::read_video_data(){
    qDebug()<<"recieve success!";
    *image =  QImage(cam_rgb_buf,width,hight,QImage::Format_RGB888);
    *pix = QPixmap::fromImage(*image).scaled(ui->label->size());

     if (frameCount >= INT_MAX)
         frameCount = 0;

     //emit sendDataTcpReady(frameCount++,width,hight,cam_rgb_buf_len,cam_rgb_buf);

     QString  str =  QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

     curDateTime = QDateTime::currentDateTime();

     if(curDateTime > preDateTime)
     {
         fps = 1000/preDateTime.msecsTo(curDateTime);
         preDateTime = curDateTime;
     }

     painter->begin(pix);
     pen.setColor(Qt::red);
     painter->setPen(pen);
     pen.setWidth(10);
     //painter->setFont(QFont("WenQuanYi Micro Hei"));

     painter->drawText(10,20,QString("frame:%1       fps:%2").arg(frameCount).arg(fps));
     painter->drawText(10,ui->label->height() - 20 ,str);
     painter->end();

     ui->label->setPixmap(*pix);
}


