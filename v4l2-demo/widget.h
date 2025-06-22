#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QString>
#include "videodevice.h"
#include <QPaintEvent>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>
#include <QDateTime>
#include <QMainWindow>
#include <QUdpSocket>


QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    int disp_box_width;
    int disp_box_height;

    QString  cam_name;
    VideoDevice *cam_vd;
    int width, hight;
    QString video_fmt;

    QTimer * timer  ;

    QPixmap *pix;
    QImage  *image;

    QPainter  *painter;
    QPen   pen;

    int fps;

    uchar     *cam_raw_buf;
    quint32    cam_raw_buf_len;
    size_t     frame_len;

    uchar     *cam_rgb_buf;
    quint32    cam_rgb_buf_len;



    bool  get_frame_flag;
    int   frameCount;

    QDateTime  preDateTime,curDateTime;

    void yuyv422_to_rgb888(unsigned char *yuyvdata, unsigned char *rgbdata, int w, int h);

signals:
    void  transData(QTcpSocket *,unsigned char *);
    void  sendDataTcpReady(int,int,int,int,unsigned char *);


private slots:
    void send_video_data();
    void read_video_data();

private:
    Ui::Widget *ui;
    QUdpSocket *server, *client;
};
#endif // WIDGET_H
