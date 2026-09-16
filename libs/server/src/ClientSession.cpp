#include "server/ClientSession.h"

// ClientSession takes ownership of socket.
ClientSession::ClientSession(QTcpSocket* socket,TicketStore* store):id(++nextId),socket(socket),store(store){
    socket->setParent(this);
    connect(socket, &QTcpSocket::readyRead,this, [this]() {onReadyRead();});
    connect(socket, &QTcpSocket::disconnected,this,[this]{
        this->store->tryCancelCheckout(id,true);
        deleteLater();
    });
}

ServerMessage ClientSession::validateClientVersion(const ClientMessage& request){
    if(versionValidated){
        if(request.message==version)return {ServerMessageType::CLIENT_WARNING_Version_already_validated};
        else {
            //just in case invalidate, but effectively redundant since crit_err will drop the connection
            versionValidated=false;
            return {ServerMessageType::CRIT_CLIENT_ERR_Version_invalidated};
        }
    }else{
        if(request.message==version){
            versionValidated=true;
            return {ServerMessageType::OK};
        }
        else{
            return {ServerMessageType::CRIT_CLIENT_ERR_Version_mismatch};
        }
    }
}

ServerMessage ClientSession::craftResponse(const ClientMessage& request){
    if(request.type==ClientMessageType::REQUEST_Version_validation){
        return validateClientVersion(request);
    }
    if(versionValidated==false){
        return {ServerMessageType::ERR_Version_not_validated};
    }
    if(request.type==ClientMessageType::REQUEST_Get_ticket_list){
        if(request.message.isEmpty()==false)return {ServerMessageType::ERR_Incorrect_argument_count};
        else return store->getTicketList();
    }
    //Format: START_CHECKOUT <ticket_name>
    if(request.type==ClientMessageType::REQUEST_Start_checkout){
        return store->tryCheckout(id,request);
    }
    //format: BUY <customer_name> <ticket_name>
    if(request.type==ClientMessageType::REQUEST_Buy){
        return store->confirmPurchase(id,request);
    }
    if(request.type==ClientMessageType::REQUEST_Cancel_checkout){
        if(request.message.isEmpty()==false)return {ServerMessageType::ERR_Incorrect_argument_count};
        else return store->tryCancelCheckout(id);
    }
    return {ServerMessageType::ERR_Unknown_Command};
}

QByteArray ClientSession::frameResponse(const ServerMessage& response,const ClientMessage& request){
    QByteArray answer;
    quint32 size=response.message.size()+sizeof(response.type)+sizeof(request.type);

    answer.reserve(size+sizeof(size));
    
    answer.append(parsing::packNumber(size));
    answer.append(parsing::packNumber((quint16)response.type));
    answer.append(parsing::packNumber((quint16)request.type));
    answer.append(response.message);
    return answer;
}

QByteArray ClientSession::buildDiagnostic(const QByteArray& prefix,const ServerMessage& error,const ClientMessage& request){
    QByteArray result = prefix;
    result+=" ErrorCode:";
    result+=QByteArray::number((quint16)error.type);
    if(error.message.isEmpty()==false){
        result+=" ErrorMessage:";
        result+=error.message;
    }
    result+=" RequestCode:";
    result+=QByteArray::number((quint16)request.type);
    if(request.message.isEmpty()==false){
        result+=" RequestMessage:";
        result+=request.message;
    }
    return result;
}

[[noreturn]] void ClientSession::fatalShutDown(const ServerMessage& error,const ClientMessage& request){
    QByteArray errorString=buildDiagnostic("CRIT_SERVER_ERROR",error,request);
    //todo: dump all possibly relevant information into a file
    //todo: send general information about shutdown to all connected sockets,then try to shutdown gracefully. 
    //placeholder:
    qFatal("%s", errorString.constData());
}


void ClientSession::fail(const ServerMessage& error,const ClientMessage& request){
    QByteArray errorString=buildDiagnostic("ERROR",error,request);
    qWarning()<<errorString;
    socket->write(frameResponse(error,request));
    socket->disconnectFromHost();
}

void ClientSession::onReadyRead() {
    buffer+=socket->readAll();

    if(buffer.size()>MAX_BUFFER_SIZE){
        fail({ServerMessageType::CRIT_CLIENT_ERR_Request_buffer_overflow},{ClientMessageType::Invalid});
        return;
    }
    while(true){
        if(buffer.size()<4)break;

        quint32 len=parsing::unpackNumber<quint32>(buffer);
        quint32 totalLen=len+4;
        if(len>MAX_REQUEST_SIZE){
            fail({ServerMessageType::CRIT_CLIENT_ERR_Exceeded_maximum_request_length},{ClientMessageType::Invalid});
            return;
        }

        if(buffer.size()<totalLen)break;
        QByteArray rawRequest = buffer.mid(4,len);
        buffer.remove(0,totalLen);

        ServerMessage response;
        ClientMessage request;
        
        
        if(rawRequest.isEmpty()){
            request  = {ClientMessageType::Invalid};
            response = {ServerMessageType::ERR_Empty_command};
        }
        else if(rawRequest.size()<2){
            request  = {ClientMessageType::Invalid};
            response = {ServerMessageType::ERR_Not_enough_bits};
        }
        else {
            request=parsing::unpackClientMessage(rawRequest);
            response = craftResponse(request);
        }

        //debug temporary!!!

        qInfo()<<id<<" :: "<< quint64(request.type)<<":"<<request.message<<"|"<<Qt::hex<<quint64(response.type)<<":"<<response.message;

        //\debug

        if(serverMessageCheckCategory(response.type,ServerMessageCategory::ClientCrit)){
            fail(response,request);
            return;
        }
        if(serverMessageCheckCategory(response.type,ServerMessageCategory::ServerCrit)){
            fatalShutDown(response,request);
            return;
        }
        if(serverMessageCheckCategory(response.type,ServerMessageCategory::Error)){
            qWarning()<<"CLIENT_ERROR code "<< (quint16)response.type <<response.message;
        }
        if(serverMessageCheckCategory(response.type,ServerMessageCategory::ClientWarning)){
            qWarning()<<"CLIENT_WARNING:"<<response.message;
        }
        socket->write(frameResponse(response,request));
    }
}
