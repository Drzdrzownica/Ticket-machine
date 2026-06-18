#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSocketNotifier>
#include <QTextStream>
#include <QList>
#include <QtEndian>


//avoid using types ending with "RESERVED". They are meant to be used as a temporary placeholder for emergency hotfixes.
enum class MessageType:quint16{
    //non-critical errors: 0x0000<=val<0x1000 
    ERROR_RESERVED=0x0000, 
    ERR_Empty_command,
    ERR_Unknown_Command,
    ERR_Version_not_validated,
    ERR_Not_enough_bits,
    ERR_Not_a_CLIENT_REQUEST_type,
    ERR_No_checkout_in_progress,
    ERR_Incorrect_argument_count,
    ERR_Checkout_with_no_arguments,
    ERR_Parsing_error_Too_many_arguments,
    ERR_Parsing_error_Unpacking_failed,
    ERR_Ticket_name_is_an_empty_string,
    ERR_Different_ticket_already_in_checkout,
    ERR_Invalid_ticket_name_during_checkout,
    ERR_No_tickets_in_stock_during_checkout,
    ERR_Empty_string_argument,
    ERR_Disallowed_name_try_again,
    ERR_Attempted_purchase_item_not_in_checkout,
    ERR_Wrong_item_in_checkout,
    ERR_Ticket_no_longer_valid_Reservation_removed,
    ERROR_END,

    //critical server errors : 0x1000<=val<0x2000
    CRIT_SERVER_ERR_RESERVED=0x1000,
    CRIT_SERVER_ERR_Ticket_list_empty,
    CRIT_SERVER_ERR_ticket_list_overflow,
    CRIT_SERVER_ERR_No_valid_tickets_found,
    CRIT_SERVER_ERR_END,

    //critical client errors - disconnect socket : 0x2000<=val<0x3000
    CRIT_CLIENT_ERR_RESERVED=0x2000,
    CRIT_CLIENT_ERR_Invalid_ticket_in_reservation,
    CRIT_CLIENT_ERR_Version_invalidated,
    CRIT_CLIENT_ERR_Client_server_version_mismatch,
    CRIT_CLIENT_ERR_Exceeded_maximum_request_length,
    CRIT_CLIENT_ERR_Request_buffer_overflow,
    CRIT_CLIENT_ERR_END,

    //client warnings : 0x3000<=val<0x4000
    CLIENT_WARNING_RESERVED=0x3000,
    CLIENT_WARNING_Ticket_already_in_checkout,
    CLIENT_WARNING_Version_already_validated,
    CLIENT_WARNING_END,

    //client requests : 0x4000<=val<0x5000
    CLIENT_REQUEST_RESERVED=0x4000,
    CLIENT_REQUEST_VERSION_VALIDATION,
    CLIENT_REQUEST_GET_TICKET_LIST,
    CLIENT_REQUEST_START_CHECKOUT,
    CLIENT_REQUEST_BUY,
    CLIENT_REQUEST_CANCEL_CHECKOUT,
    CLIENT_REQUEST_END,

    //request succeeded responses : 0x5000<=val<0x6000
    OK_RESERVED=0x5000,
    OK_CANCEL_CHECKOUT_checkout_empty,
    OK_CANCEL_CHECKOUT_checkout_cancelled,
    OK_GET_TICKET_LIST,
    OK_START_CHECKOUT,
    OK_BUY,
    OK_VERSION_VALIDATION,
    OK_END
};


static_assert((quint16)MessageType::ERROR_END<0x1000);
static_assert((quint16)MessageType::CRIT_SERVER_ERR_END<0x2000);
static_assert((quint16)MessageType::CRIT_CLIENT_ERR_END<0x3000);
static_assert((quint16)MessageType::CLIENT_WARNING_END<0x4000);
static_assert((quint16)MessageType::CLIENT_REQUEST_END<0x5000);
static_assert((quint16)MessageType::OK_END<0x6000);

constexpr quint16 CATEGORY_MASK = 0xF000;
enum class MessageCategory : quint16 {
    Error         = 0x0000,
    ServerCrit    = 0x1000,
    ClientCrit    = 0x2000,
    ClientWarning = 0x3000,
    ClientRequest = 0x4000,
    Ok            = 0x5000
};


bool isMessageTypeError(MessageType type){
    return (static_cast<quint16>(type) & CATEGORY_MASK) == static_cast<quint16>(MessageCategory::Error);
}
bool isMessageTypeCritClientError(MessageType type){
    return (static_cast<quint16>(type) & CATEGORY_MASK) == static_cast<quint16>(MessageCategory::ClientCrit);
}
bool isMessageTypeCritServerError(MessageType type){
    return (static_cast<quint16>(type) & CATEGORY_MASK) == static_cast<quint16>(MessageCategory::ServerCrit);
}
bool isMessageTypeClientWarning(MessageType type){
    return (static_cast<quint16>(type) & CATEGORY_MASK) == static_cast<quint16>(MessageCategory::ClientWarning);
}
bool isMessageTypeClientRequest(MessageType type){
    return (static_cast<quint16>(type) & CATEGORY_MASK) == static_cast<quint16>(MessageCategory::ClientRequest);
}
struct Message{
    MessageType type;
    QByteArray message="";//optional
};

namespace parsing{
    QByteArray pack8BitPrefixedByteArray(const QByteArray& array){
        QByteArray result;
        if(array.size()>255)throw std::invalid_argument("array_too_long");
        result.append(static_cast<quint8>(array.size()));
        result.append(array);
        return result;
    }

    QByteArray unpack8BitPrefixedByteArray(const QByteArray& parameters,quint32& offset){
        QByteArray answer;
        if(parameters.size()<=offset)throw std::out_of_range("Offset_larger_than_parameters_size");
        quint8 len=parameters[offset++];
        if(len>parameters.size()-offset)throw std::out_of_range("Prefix_larger_than_remaining_message");
        quint32 oldOffset=offset;
        offset+=len;
        return parameters.mid(oldOffset,len);
    }

    template<typename T>
    requires std::is_integral_v<T> && std::is_unsigned_v<T>
    QByteArray packNumber(T number){
        number = qToBigEndian(number);
        QByteArray result;
        result.append(reinterpret_cast<const char*>(&number),sizeof(number));
        return result; 
    }

    quint16 unpack16BitNumber(const QByteArray& array,quint32 index=0){
        if(index+2>array.size())throw std::out_of_range("Not_enough_bits_left");
        return qFromBigEndian<quint16>(reinterpret_cast<const uchar*>(array.constData()+index));
    }
    quint32 unpack32BitNumber(const QByteArray& array,quint32 index=0){
        if(index+4>array.size())throw std::out_of_range("Not_enough_bits_left");
        return qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(array.constData()+index));
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
        if(tickets.empty())return{MessageType::CRIT_SERVER_ERR_Ticket_list_empty};
        
        //todo: Subscribe socket to ticket availability updates.

        QByteArray answer;
        
        if(tickets.size()>255)return {MessageType::CRIT_SERVER_ERR_ticket_list_overflow};
        quint8 validTickets=0;
        //reserve space for validTickets;
        answer.append('\0');

        for(auto& [name,data]:tickets){
            QByteArray ticketPacketData;
            try{
                ticketPacketData.append(parsing::pack8BitPrefixedByteArray(name));
                ticketPacketData.append(parsing::packNumber(data.cost));
                //only if it's available - yes/no.
                ticketPacketData.append((data.availableAmount>0) ? 1 : 0);
            }catch(std::exception& e){
                qWarning()<<"SERVER_ERROR failed_to_process_ticket:"<<e.what();
                continue;
            }
            validTickets++;
            answer.append(ticketPacketData);
        }
        if(validTickets==0)return {MessageType::CRIT_SERVER_ERR_No_valid_tickets_found};
        answer.data()[0]=validTickets;
        return {MessageType::OK_GET_TICKET_LIST,answer};
    }

    Message tryCancelCheckout(ClientSession* socket,bool disconnectCleanup=false){
        //todo: connect so it runs 5 minutes after successful tryCheckout and sends such information through the socket, unless called or canceled by buy.

        auto currentCheckout=inCheckout.find(socket);
        if(currentCheckout==inCheckout.end()){
            if(disconnectCleanup)return {MessageType::OK_CANCEL_CHECKOUT_checkout_empty};
            else return {MessageType::ERR_No_checkout_in_progress};
        }
        auto currentTicket = tickets.find(currentCheckout->second);
        if(currentTicket==tickets.end()){
            inCheckout.erase(socket);
            return {MessageType::CRIT_CLIENT_ERR_Invalid_ticket_in_reservation};
        }

        currentTicket->second.availableAmount++;
        inCheckout.erase(socket);
        return {MessageType::OK_CANCEL_CHECKOUT_checkout_cancelled};
    }

    Message tryCheckout(ClientSession* socket,const QByteArray& arguments){

        if(arguments.isEmpty())return {MessageType::ERR_Checkout_with_no_arguments};
        
        QByteArray ticketName;
        try{
            quint32 offset=0;
            ticketName=parsing::unpack8BitPrefixedByteArray(arguments,offset);
            if(offset!=arguments.size())return {MessageType::ERR_Parsing_error_Too_many_arguments};
        }catch(std::exception& e){
            return {MessageType::ERR_Parsing_error_Unpacking_failed};
        }
        if(ticketName.isEmpty())return {MessageType::ERR_Ticket_name_is_an_empty_string};


        auto checkout = inCheckout.find(socket);
        if(checkout!=inCheckout.end()){
            if(checkout->second==ticketName)return {MessageType::CLIENT_WARNING_Ticket_already_in_checkout};
            else return {MessageType::ERR_Different_ticket_already_in_checkout};
        }
        auto ticket = tickets.find(ticketName);
        if(ticket==tickets.end())return {MessageType::ERR_Invalid_ticket_name_during_checkout};
        
        if(ticket->second.availableAmount>0){
            inCheckout.emplace(socket,ticketName);
            ticket->second.availableAmount--;
            return {MessageType::OK_START_CHECKOUT,ticketName};
        }else{
            return {MessageType::ERR_No_tickets_in_stock_during_checkout};
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
            if(offset!=parameters.size())return {MessageType::ERR_Parsing_error_Too_many_arguments};
        }catch(std::exception& e){
            return {MessageType::ERR_Parsing_error_Unpacking_failed};
        }
        if(buyerName.isEmpty() || ticketName.isEmpty())return {MessageType::ERR_Empty_string_argument};

        auto reservation = inCheckout.find(socket);
        //keep item in checkout unless error specifies otherwise
        if(validateName(buyerName)==false)return {MessageType::ERR_Disallowed_name_try_again};
        if(reservation==inCheckout.end())return {MessageType::ERR_Attempted_purchase_item_not_in_checkout};
        if(reservation->second!=ticketName)return {MessageType::ERR_Wrong_item_in_checkout};
        if(tickets.find(ticketName)==tickets.end()){
            inCheckout.erase(socket);
            return {MessageType::ERR_Ticket_no_longer_valid_Reservation_removed};
        }
        //todo: stop the 5 minutes cancel-checkout clock
        //todo: Push [name][ticket_name] into the database. On fail return error. For now as a placeholder:
        qInfo()<<buyerName+" purchased ticket for "+ticketName;
        inCheckout.erase(socket);
        
        return {MessageType::OK_BUY,ticketName};
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
            if(request==version)return {MessageType::CLIENT_WARNING_Version_already_validated};
            else {
                //just in case invalidate, but effectively redundant since crit_err will drop the connection
                versionValidated=false;
                return {MessageType::CRIT_CLIENT_ERR_Version_invalidated};
            }
        }else{
            if(request==version){
                versionValidated=true;
                return {MessageType::OK_VERSION_VALIDATION};
            }
            else{
                return {MessageType::CRIT_CLIENT_ERR_Client_server_version_mismatch};
            }
        }
    }

    Message craftResponse(const QByteArray& request){
        if(request.isEmpty())return {MessageType::ERR_Empty_command};
        if(request.size()<2)return {MessageType::ERR_Not_enough_bits};

        MessageType opType = static_cast<MessageType>(parsing::unpack16BitNumber(request));
        if(isMessageTypeClientRequest(opType)==false)return {MessageType::ERR_Not_a_CLIENT_REQUEST_type};

        if(opType==MessageType::CLIENT_REQUEST_VERSION_VALIDATION){
            return validateClientVersion(request.mid(2));
        }

        if(versionValidated==false){
            return {MessageType::ERR_Version_not_validated};
        }

        if(opType==MessageType::CLIENT_REQUEST_GET_TICKET_LIST){
            if(request.size()!=2)return {MessageType::ERR_Incorrect_argument_count};
            else return store->getTicketList();
        }
        //Format: START_CHECKOUT <ticket_name>
        if(opType==MessageType::CLIENT_REQUEST_START_CHECKOUT){
            return store->tryCheckout(this,request.mid(2));
        }
        //format: BUY <customer_name> <ticket_name>
        if(opType==MessageType::CLIENT_REQUEST_BUY){
            return store->confirmPurchase(this,request.mid(2));
        }
        if(opType==MessageType::CLIENT_REQUEST_CANCEL_CHECKOUT){
            if(request.size()!=2)return {MessageType::ERR_Incorrect_argument_count};
            else return store->tryCancelCheckout(this);
        }
        return {MessageType::ERR_Unknown_Command};
    }

    QByteArray frameResponse(const Message& response){

        QByteArray answer;
        quint32 size=response.message.size()+sizeof(response.type);
        answer.reserve(response.message.size()+sizeof(response.type)+sizeof(size));
        
        answer.append(parsing::packNumber(size));
        answer.append(parsing::packNumber((quint16)response.type));
        answer.append(response.message);
        return answer;
    }

    void fail(Message message,const QByteArray& request=""){
        QByteArray error;
        if(isMessageTypeCritServerError(message.type)){
            //todo: dump all possibly relevant information into a file
            error="CRIT_SERVER_ERROR. ErrorCode:";
            error+=QByteArray::number((quint16)message.type);
            if(message.message.isEmpty()==false){
                error+=" ErrorMessage:";
                error+=message.message;
            }
            //todo: send general information about shutdown to all connected sockets(not server error),then try to shutdown gracefully. For now as a placeholder we do it the quick way:
            qFatal("%s", error.constData());
        }

        error="ERROR. ErrorCode:";
        error+=QByteArray::number((quint16)message.type);

        if(message.message.isEmpty()==false){
            error+=" ErrorMessage:";
            error+=message.message;
        }

        if(request.isEmpty()==false){
            error+=" OnRequestCode:";
            error+=QByteArray::number(parsing::unpack16BitNumber(request));
            if(request.size()>2){
                error+=" RequestMessage:";
                error+=request.mid(2);
            }
        }
        qWarning()<<error;
        socket->write(frameResponse(message));
        socket->flush();
        socket->disconnectFromHost();
    }

    void onReadyRead() {
        buffer+=socket->readAll();

        if(buffer.size()>MAX_BUFFER_SIZE){
            fail({MessageType::CRIT_CLIENT_ERR_Request_buffer_overflow});
            return;
        }
        while(true){
            if(buffer.size()<4)break;

            quint32 len=parsing::unpack32BitNumber(buffer);
            quint32 totalLen=len+4;
            if(len>MAX_REQUEST_SIZE){
                fail({MessageType::CRIT_CLIENT_ERR_Exceeded_maximum_request_length},buffer);
                return;
            }
            if(buffer.size()<totalLen)break;
            QByteArray request = buffer.mid(4,len);
            buffer.remove(0,totalLen);

            Message response = craftResponse(request);
            if(isMessageTypeCritClientError(response.type)){
                fail(response,request);
                return;
            }
            if(isMessageTypeCritServerError(response.type)){
                fail(response,request);
                return;
            }
            if(isMessageTypeError(response.type)){
                qWarning()<<"CLIENT_ERROR code "<< (quint16)response.type <<response.message;
            }
            if(isMessageTypeClientWarning(response.type)){
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