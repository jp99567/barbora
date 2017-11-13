#include "tcpconnection.h"

#include <QTcpSocket>
#include <QDebug>

using lbidich::ChannelId;



TcpConnection::TcpConnection()
    :channelDown(ChannelId::down, *this)
    ,channelUp(ChannelId::up, *this)
    ,transport({ {ChannelId::down, boost::shared_ptr<apache::thrift::transport::TTransport>(new BytTransport(channelDown))},
                 {ChannelId::up,    boost::shared_ptr<apache::thrift::transport::TTransport>(new   BytTransport(channelUp))} } )
{
    socket = std::unique_ptr<QAbstractSocket>(new QTcpSocket(this));
    connect(socket.get(), &QAbstractSocket::stateChanged, this, &TcpConnection::stateChanged);
    connect(this, &TcpConnection::writeReq, this, &TcpConnection::writeReqSlot, Qt::ConnectionType::QueuedConnection);
    connect(socket.get(), &QAbstractSocket::connected, [this]{
        writeReqSlot(lbidich::packMsg2((uint8_t)ChannelId::auth, "Na Straz!"));
    });
}

void TcpConnection::writeReqSlot(lbidich::DataBuf data)
{
    qDebug() << "writeReqSlot" << data.size();
        if(dataWr.size() > 0){
            std::copy(std::cbegin(data), std::cend(data), std::back_inserter(dataWr));
            return;
        }

        auto written = socket->write((const char*)data.data(), data.size());
        qDebug() << "socket->write" << written;
        if(written == -1)
            return;

        if(written < (qint64)data.size())
        {
            std::copy(std::cbegin(data)+written, std::cend(data), std::back_inserter(dataWr));
        }
        return; //TODO
}

TcpConnection::~TcpConnection()
{
    qDebug() << "destroy" << "~TcpConnection";
}

boost::shared_ptr<apache::thrift::transport::TTransport> TcpConnection::getClientChannel()
{
    return transport[ChannelId::down];
}

bool TcpConnection::put(lbidich::ChannelId chId, const uint8_t *msg, unsigned len)
{
    emit writeReq(lbidich::packMsg((uint8_t)chId, msg, len));
    return true;
}

void TcpConnection::connectTo() {
    socket->connectToHost("127.0.0.1", 1981);
}

void TcpConnection::stateChanged(QAbstractSocket::SocketState state)
{
    qDebug() << state;
}
