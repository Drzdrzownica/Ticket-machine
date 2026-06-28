#include "server/ClientSession.h"
#include <QTcpServer>

int main(int argc, char* argv[]){
    
    QCoreApplication app(argc,argv);
    TicketStore store;
    QTcpServer server;

    QObject::connect(&server,&QTcpServer::newConnection,[&server,&store](){
        while(server.hasPendingConnections()){
            new ClientSession(server.nextPendingConnection(),&store);
        }
    });
    if(!server.listen(QHostAddress::LocalHost,12345)){
        qFatal("Failed to start the server: %s", qPrintable(server.errorString()));
    }
    return app.exec();
}
