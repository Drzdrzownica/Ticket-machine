#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSocketNotifier>
#include <QTextStream>
#include <QList>
#include <QtEndian>

enum class MessageType:quint8{
    OK=0x00,
    UPDATE=0x01,
    VERSION_VALIDATION=0x02,
    ERR=0x10,
    CRIT_CLIENT_ERR=0x11,
    CRIT_SERVER_ERR=0x12,
    CLIENT_WARNING=0x13,
    GET_TICKET_LIST=0x20,
    START_CHECKOUT=0x21,
    CANCEL_CHECKOUT=0x22,
    BUY=0x23
};

struct Message{
    MessageType type;
    QByteArray message;
};

namespace parsing{
    QByteArray unpack8BitPrefixedByteArray(const QByteArray& parameters,quint32& offset){
        QByteArray answer;
        if(parameters.size()<=offset)throw std::out_of_range("Offset_larger_than_parameters_size");
        quint8 len=parameters[offset++];
        if(len>parameters.size()-offset)throw std::out_of_range("Prefix_larger_than_remaining_message");
        int oldOffset=offset;
        offset+=len;
        return parameters.mid(oldOffset,len);
    }
    QByteArray pack8BitPrefixedByteArray(const QByteArray& array){
        QByteArray answer;
        if(array.size()>255)throw std::invalid_argument("array_too_long");
        answer.append(static_cast<quint8>(array.size()));
        answer.append(array);
        return answer;
    }
    QByteArray pack32BitNumber(quint32 number){
        number = qToBigEndian(number);
        QByteArray answer;
        answer.append(reinterpret_cast<const char*>(&number),sizeof(number));
        return answer; 
    }
}

class ClientSession;

class TicketStore{
    struct Ticket{
        quint32 cost;
        quint32 availableAmount;
        Ticket(quint32 cost,quint32 amount):cost(cost),availableAmount(amount){};
        Ticket()=default;
    };
    std::map<QByteArray,Ticket> tickets;
    std::map<ClientSession*,QByteArray> inCheckout;
public:
    TicketStore(){
        //pull tickets from database
        //log and rejects invalid ticket lengths or other issues
        //placeholder:
        tickets.emplace("Lorem_Ipsum",Ticket{123,10});
        tickets.emplace("Dolor_Sit_Amet",Ticket{111,0});

        if(tickets.size()>255)qFatal("Failed to start the server: Loaded too many tickets from the database");
        if(tickets.size()>10)qWarning()<<"SERVER_WARNING: unusually large amount of tickets loaded";
        if(tickets.empty())qFatal("Failed to start the server: Failed to load tickets from the dataBase");
    }

    Message getTicketList(){
        if(tickets.empty())return{MessageType::CRIT_SERVER_ERR,"Ticket_list_empty"};
        
        //todo: Subscribe socket to ticket availability updates.

        QByteArray answer;
        
        if(tickets.size()>255)return {MessageType::CRIT_SERVER_ERR,"ticket_list_overflow"};
        quint8 validTickets=0;
        //reserve space for validTickets;
        answer.append('\0');

        for(auto& [name,data]:tickets){
            QByteArray ticketPacketData;
            try{
                ticketPacketData.append(parsing::pack8BitPrefixedByteArray(name));
                ticketPacketData.append(parsing::pack32BitNumber(data.cost));
                //only if it's available - yes/no.
                ticketPacketData.append((data.availableAmount>0) ? 1 : 0);
            }catch(std::exception& e){
                qWarning()<<"SERVER_ERROR failed_to_process_ticket:"<<e.what();
                continue;
            }
            validTickets++;
            answer.append(ticketPacketData);
        }
        if(validTickets==0)return {MessageType::CRIT_SERVER_ERR,"No_valid_tickets_found"};
        answer.data()[0]=validTickets;
        return {MessageType::OK,answer};
    }

    Message tryCancelCheckout(ClientSession* socket,bool disconnectCleanup=false){
        //todo: connect so it runs 5 minutes after successful tryCheckout and sends such information through the socket, unless called or canceled by buy.

        auto currentCheckout=inCheckout.find(socket);
        if(currentCheckout==inCheckout.end()){
            if(disconnectCleanup)return {MessageType::OK,"CancelCheckout_checkout_empty"};
            else return {MessageType::ERR,"No_checkout_in_progress"};
        }
        auto currentTicket = tickets.find(currentCheckout->second);
        if(currentTicket==tickets.end()){
            inCheckout.erase(socket);
            return {MessageType::CRIT_CLIENT_ERR,"invalid_ticket_in_reservation"};
        }

        currentTicket->second.availableAmount++;
        inCheckout.erase(socket);
        return {MessageType::OK,"CancelCheckout"};
    }

    Message tryCheckout(ClientSession* socket,const QByteArray& arguments){

        if(arguments.isEmpty())return {MessageType::ERR,"checkout_with_no_arguments_attempted"};
        
        QByteArray ticketName;
        try{
            quint32 offset=0;
            ticketName=parsing::unpack8BitPrefixedByteArray(arguments,offset);
            if(offset!=arguments.size())return {MessageType::ERR,"Parsing_error:Too_many_arguments"};
        }catch(std::exception& e){
            QByteArray errorMessage = "Parsing_error:";
            errorMessage.append(e.what());
            return {MessageType::ERR,errorMessage};
        }
        if(ticketName.isEmpty())return {MessageType::ERR,"Empty_string"};


        auto checkout = inCheckout.find(socket);
        if(checkout!=inCheckout.end()){
            if(checkout->second==ticketName)return {MessageType::CLIENT_WARNING,"Ticket_already_in_checkout"};
            else return {MessageType::ERR,"Different_ticket_already_in_checkout"};
        }
        auto ticket = tickets.find(ticketName);
        if(ticket==tickets.end())return {MessageType::ERR,"invalid_ticket_name_during_checkout"};
        
        if(ticket->second.availableAmount>0){
            inCheckout.emplace(socket,ticketName);
            ticket->second.availableAmount--;
            return {MessageType::OK,"Checkout:"+ticketName};
        }else{
            return {MessageType::ERR,"No_tickets_in_stock_during_checkout"};
        }
    }

    //For now as a placeholder rule only allow ascii letters and no spaces or special characters.
    bool validateName(const QByteArray& buyerName){
        if(buyerName.isEmpty())return false;
        for(auto character:buyerName){
            if(character>='a' && character<='z')continue;
            if(character>='A' && character<='Z')continue;
            return false;
        }
        return true;
    }

    Message confirmPurchase(ClientSession* socket,const QByteArray& parameters){

        QByteArray buyerName;
        QByteArray ticketName;
        try{
            quint32 offset=0;
            buyerName=parsing::unpack8BitPrefixedByteArray(parameters,offset);
            ticketName=parsing::unpack8BitPrefixedByteArray(parameters,offset);
            if(offset!=parameters.size())return {MessageType::ERR,"Parsing_error:Too_many_arguments"};
        }catch(std::exception& e){
            QByteArray errorMessage = "Parsing_error:";
            errorMessage.append(e.what());
            return {MessageType::ERR,errorMessage};
        }
        if(buyerName.isEmpty() || ticketName.isEmpty())return {MessageType::ERR,"Empty_string"};

        auto reservation = inCheckout.find(socket);
        //keep item in checkout unless error specifies otherwise
        if(validateName(buyerName)==false)return {MessageType::ERR,"Disallowed_name_try_again"};
        if(reservation==inCheckout.end())return {MessageType::ERR , "Attempted_to_purchase_item_not_in_checkout"};
        if(reservation->second!=ticketName)return {MessageType::ERR, "Wrong_item_in_checkout_during_purchase"};
        if(tickets.find(ticketName)==tickets.end()){
            inCheckout.erase(socket);
            return {MessageType::ERR, "Ticket_no_longer_valid_Reservation_removed"};
        }
        //todo: stop the 5 minutes cancel-checkout clock
        //todo: Push [name][ticket_name] into the database. On fail return error. For now as a placeholder:
        qInfo()<<buyerName+" purchased ticket for "+ticketName;
        inCheckout.erase(socket);
        
        return {MessageType::OK,"Buy "+ticketName};
    }
};

class ClientSession : public QObject{
public:
    // ClientSession takes ownership of socket.
    explicit ClientSession(QTcpSocket* socket,TicketStore* store):socket(socket),store(store){
        socket->setParent(this);
        connect(socket, &QTcpSocket::readyRead,this, [this]() {onReadyRead();});
        connect(socket, &QTcpSocket::disconnected,this,[this]{
            this->store->tryCancelCheckout(this,true);
            deleteLater();
        });
    }

private:
    //placeholder const values
    static constexpr int MAX_REQUEST_SIZE = 4096;
    static constexpr int MAX_BUFFER_SIZE = MAX_REQUEST_SIZE*10;
    static constexpr QByteArrayView version = "0.0.0.1";



    bool versionValidated=false;
    QTcpSocket* socket;
    QByteArray buffer;
    TicketStore* store; //non-owning

    Message validateClientVersion(const QByteArray& request){
        if(versionValidated){
            if(request==version)return {MessageType::CLIENT_WARNING,"Version_already_correctly_validated"};
            else {
                //just in case invalidate, but effectively redundant since crit_err will drop the connection
                versionValidated=false;
                return {MessageType::CRIT_CLIENT_ERR,"Version_invalidated"};
            }
        }else{
            if(request==version){
                versionValidated=true;
                return {MessageType::OK,"version_validation"};
            }
            else{
                return {MessageType::CRIT_CLIENT_ERR,"Client_server_version_mismatch"};
            }
        }
    }

    Message craftResponse(const QByteArray& request){
        if(request.isEmpty())return {MessageType::ERR,"Empty_command"};

        quint8 opCode=static_cast<quint8>(request[0]);

        if(opCode==static_cast<quint8>(MessageType::VERSION_VALIDATION)){
            return validateClientVersion(request.mid(1));
        }

        if(versionValidated==false){
            return {MessageType::ERR,"Version_match_not_validated"};
        }

        if(opCode==static_cast<quint8>(MessageType::GET_TICKET_LIST)){
            if(request.size()!=1)return {MessageType::ERR,"Incorrect_argument_count tickets"};
            else return store->getTicketList();
        }
        //format: START_CHECKOUT <ticket_name>
        if(opCode==static_cast<quint8>(MessageType::START_CHECKOUT)){
            return store->tryCheckout(this,request.mid(1));
        }
        //format: BUY <customer_name> <ticket_name>
        if(opCode==static_cast<quint8>(MessageType::BUY)){
            return store->confirmPurchase(this,request.mid(1));
        }
        if(opCode==static_cast<quint8>(MessageType::CANCEL_CHECKOUT)){
            if(request.size()!=1)return {MessageType::ERR,"Incorrect_argument_count cancel_checkout"};
            else return store->tryCancelCheckout(this);
        }
        return {MessageType::ERR,"Unknown_Command"};
    }

    QByteArray frameResponse(const Message& response){

        QByteArray answer;
        quint32 size=response.message.size()+1;
        quint32 endianSize=qToBigEndian(size);

        answer.reserve(response.message.size()+1+sizeof(endianSize));
        
        answer.append(reinterpret_cast<const char*>(&endianSize),sizeof(endianSize));
        answer.append(static_cast<quint8>(response.type));
        answer.append(response.message);

        return answer;
    }

    void fail(Message message,const QByteArray& request=""){
        QByteArray error;
        if(message.type==MessageType::CRIT_CLIENT_ERR)error="CRIT_ERROR "+message.message;
        else if(message.type==MessageType::CRIT_SERVER_ERR){
            //todo: dump all possibly relevant information into a file
            error="CRIT_SERVER_ERROR "+message.message;
            //todo: send general information about shutdown to all connected sockets(not server error),then try to shutdown gracefully. For now we do it the quick way:
            qFatal("%s", error.constData());
        }
        else error="UNRECOGNIZED_TYPE_ERROR "+message.message;
        
        if(request.isEmpty())qWarning()<<error;
        else qWarning()<<error<<" on request "<<request.mid(1);
        socket->write(frameResponse(message));
        socket->flush();
        socket->disconnectFromHost();
    }

    void onReadyRead() {
        buffer+=socket->readAll();

        if(buffer.size()>MAX_BUFFER_SIZE){
            fail({MessageType::CRIT_CLIENT_ERR,"Connection_dropped_Request_buffer_overflow"});
            return;
        }
        while(true){
            if(buffer.size()<4)break;

            QDataStream dataStreamForLenRead(&buffer,QIODevice::ReadOnly);
            dataStreamForLenRead.setByteOrder(QDataStream::BigEndian);
            quint32 len;
            dataStreamForLenRead>>len;
            quint32 totalLen=len+4;
            if(len>MAX_REQUEST_SIZE){
                fail({MessageType::CRIT_CLIENT_ERR,"Connection_dropped_Exceeded_maximum_request_length"},buffer);
                return;
            }
            if(buffer.size()<totalLen)break;
            QByteArray request = buffer.mid(4,len);
            buffer.remove(0,totalLen);

            Message response = craftResponse(request);
            if(response.type==MessageType::CRIT_CLIENT_ERR){
                fail(response,request);
                return;
            }
            if(response.type==MessageType::ERR){
                qWarning()<<"CLIENT_ERROR:"<<response.message;
            }
            if(response.type==MessageType::CLIENT_WARNING){
                qWarning()<<"CLIENT_WARNING:"<<response.message;
            }
            socket->write(frameResponse(response));
        }
    }
};

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