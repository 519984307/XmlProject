#include "clientthread.h"
#include "protocol.h"
#include <windows.h>
#include <QtNetwork>



ClientThread::ClientThread()
{
    m_addr = "192.168.174.133";
    m_port = 4000;
    m_tcpClient = NULL;
    //start();
}

void ClientThread::run()
{
    qDebug() << "thread id:" << currentThreadId();
    if(m_tcpClient == NULL)
    {
        m_tcpClient = new QTcpSocket(this);
        connect(m_tcpClient,SIGNAL(connected()),this,SLOT(receiveConnected()));
        connect(m_tcpClient,SIGNAL(readyRead()),this,SLOT(read_Data()));
        //void QAbstractSocket::error ( QAbstractSocket::SocketError socketError )
        connect(m_tcpClient,SIGNAL( error ( QAbstractSocket::SocketError socketError ) ),this,SLOT(receiveConnectedError(QTcpSocket::SocketError socketError)));
        //Qt socket 连接是异步的
        m_tcpClient->connectToHost(QHostAddress(m_addr),m_port);
        //查看当前的状态
        qDebug("State:%d\n",m_tcpClient->state());
        if(false == m_tcpClient->isValid())
        {
            qDebug() << "socket 无效！" << endl;
            //这里会有内存泄漏，先不急处理
            m_tcpClient->close();
        }
        const int Timeout =5*1000;
        if(!m_tcpClient->waitForConnected(Timeout))
        {
             qDebug() << "socket 未连接！" << endl;
             m_tcpClient->close();
        }
        qDebug("State:%d\n",m_tcpClient->state());
        //Sleep(100000);
    }
}

void ClientThread::read_Data()
{
    //这里服务端发送了2个包只接收到了一个包，存在粘包问题
    qDebug() << tr("接收到数据包了！");
    if (!m_tcpClient->waitForReadyRead(30000)){
            qDebug() << "waitForReadyRead() timed out";
            return;
    }
    QByteArray m_data = m_tcpClient->readAll();
    Protocol *p = (Protocol*)m_data.data();
    switch(p->event)
    {
    //查询路线的返回包
    case 2:
    {
        FindRouteAck routeAck;
        memcpy(&routeAck,(char*)p+sizeof(Protocol),sizeof(FindRouteAck));
        if(-1 == routeAck.distant)
        {
            qDebug() << tr("未查找到该条线路") << endl;
            break;
        }
        emit changeRouteText(QString(routeAck.szRouteBuf));
    }
    case 4:
    {

    }
    default:
    {
        qDebug() << tr("未找到该数据包") << endl;
    }
    }

    //QDataStream

}

void ClientThread::receiveConnected()
{
    qDebug() << tr("连接被建立!") << endl;
}

void ClientThread::receiveConnectedError(QTcpSocket::SocketError socketError)
{
    qDebug() << tr("连接建立失败!") << endl;
}



void ClientThread::sendData(const QString &start, const QString &end)
{
    qDebug() << tr("开始发送数据") << endl;
    Protocol *pStr = (Protocol*)malloc(sizeof(Protocol)+ sizeof(FindRouteRequest));
    FindRouteRequest RouteData;
    pStr->event = FINDROUTE_REQUEST;
    pStr->nsize = sizeof(FindRouteRequest);
    strcpy(RouteData.szStart,start.toStdString().c_str());
    strcpy(RouteData.szEnd,end.toStdString().c_str());
    memcpy((char*)pStr+sizeof(Protocol),(char*)&RouteData,sizeof(FindRouteRequest));
    qint64 len = sizeof(Protocol)+sizeof(FindRouteRequest);
    qint64 ret =  m_tcpClient->write((char*)pStr,len);
    //这里要加这一句话，因为Qt 的连接是异步的
    m_tcpClient->waitForBytesWritten(300);
    qDebug("State:%d\n",m_tcpClient->state());  // State: 3（ConnectedState）正确
    if(-1 == ret)
    {
        qDebug() << tr("发送消息失败!") << endl;
    }
    free(pStr);
    qDebug() << tr("发送数据完成") << endl;

}
