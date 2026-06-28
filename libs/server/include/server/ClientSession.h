#ifndef CLIENT_SESSION_H
#define CLIENT_SESSION_H
#include "TicketStore.h"
#include <QTcpSocket>


class ClientSession : public QObject{
public:
    const quint64 id;
    // ClientSession takes ownership of socket.
    explicit ClientSession(QTcpSocket* socket,TicketStore* store);

private:
    //placeholder const values
    static constexpr int MAX_REQUEST_SIZE = 4096;
    static constexpr int MAX_BUFFER_SIZE = MAX_REQUEST_SIZE*10;
    static constexpr QByteArrayView version = "0.0.0.1";

    static inline std::atomic<quint64> nextId{0};

    bool versionValidated=false;
    QTcpSocket* socket;
    QByteArray buffer;
    TicketStore* store; //non-owning

    ServerMessage validateClientVersion(const ClientMessage& request);
    ServerMessage craftResponse(const ClientMessage& request);
    QByteArray frameResponse(const ServerMessage& response,const ClientMessage& request);
    QByteArray buildDiagnostic(const QByteArray& prefix,const ServerMessage& error,const ClientMessage& request);
    [[noreturn]] void fatalShutDown(const ServerMessage& error,const ClientMessage& request);
    void fail(const ServerMessage& error,const ClientMessage& request);
    void onReadyRead();
};
#endif
