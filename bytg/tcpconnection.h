#pragma once

#include <memory>
#include <queue>
#include <map>
#include <functional>
#include <condition_variable>
#include <QAbstractSocket>
#include "lbidich/io.h"
#include "lbidich/packet.h"
#include "lbidich/channel.h"
#include "lbidich/byttransport.h"

class TcpConnection;

class Io : public lbidich::IoBase, public std::enable_shared_from_this<Io>
{
public:
    decltype(dataWr)& getDataWr() {return dataWr;}
    bool put(lbidich::ChannelId chId, const uint8_t *msg, unsigned len) override;

    static std::shared_ptr<Io> create(TcpConnection& tcp)
    {
        return std::shared_ptr<Io>(new Io(tcp));
    }
    
    bool onNewPacket(lbidich::ChannelId ch, lbidich::DataBuf msg) override;

private:
   Io(TcpConnection& tcp):tcp(tcp){}
   TcpConnection& tcp;
};

class TcpConnection : public QObject
{
    Q_OBJECT
public:
    TcpConnection();

    ~TcpConnection();

    std::shared_ptr<apache::thrift::transport::TTransport> getClientChannel();

    bool put(lbidich::DataBuf msg);

public slots:
    void connectTo(QString host);

signals:
    void resultReady(const QString &result);
    void writeReq(lbidich::DataBuf data);
    void connectionChanged(QString state);

private:
    QAbstractSocket* socket;
    std::shared_ptr<Io> io;
    char dataRd[256];

private slots:
    void stateChanged(QAbstractSocket::SocketState state);
    void writeReqSlot(lbidich::DataBuf data);
    void readReadySlot();
};
