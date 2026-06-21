#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSocketNotifier>
#include <QTextStream>
#include <QList>
#include <QtEndian>

//While still in active development, don't fix the enum values yet, as contents of below enums are still very dynamic. Give them explicit values before deployment.
enum class ServerMessageType:quint16{
    //non-critical errors: 0x0000<=val<0x1000 
    ERROR_BEGIN_RESERVED=0x0000, 
    ERR_Empty_command,
    ERR_Unknown_Command,
    ERR_Version_not_validated,
    ERR_Not_enough_bits,
    ERR_No_checkout_in_progress,
    ERR_Incorrect_argument_count,
    ERR_Checkout_with_no_arguments,
    ERR_Parsing_error,
    ERR_Ticket_name_empty,
    ERR_Different_ticket_already_in_checkout,
    ERR_Invalid_ticket_name,
    ERR_No_tickets_in_stock_during_checkout,
    ERR_Empty_string_argument,
    ERR_Disallowed_name_try_again,
    ERR_Item_not_in_checkout,
    ERR_Wrong_item_in_checkout,
    ERR_Ticket_no_longer_valid, //automatically removes reservation
    ERROR_END_RESERVED,

    //critical server errors : 0x1000<=val<0x2000
    CRIT_SERVER_ERR_BEGIN_RESERVED=0x1000,
    CRIT_SERVER_ERR_Ticket_list_empty,
    CRIT_SERVER_ERR_Ticket_list_overflow,
    CRIT_SERVER_ERR_No_valid_tickets_found,
    CRIT_SERVER_ERR_END_RESERVED,

    //critical client errors - always disconnect socket : 0x2000<=val<0x3000
    CRIT_CLIENT_ERR_BEGIN_RESERVED=0x2000,
    CRIT_CLIENT_ERR_Invalid_ticket_in_reservation,
    CRIT_CLIENT_ERR_Version_invalidated,
    CRIT_CLIENT_ERR_Version_mismatch,
    CRIT_CLIENT_ERR_Exceeded_maximum_request_length,
    CRIT_CLIENT_ERR_Request_buffer_overflow,
    CRIT_CLIENT_ERR_END_RESERVED,

    //client warnings : 0x3000<=val<0x4000
    CLIENT_WARNING_BEGIN_RESERVED=0x3000,
    CLIENT_WARNING_Ticket_already_in_checkout,
    CLIENT_WARNING_Version_already_validated,
    CLIENT_WARNING_END_RESERVED,

    //request succeeded responses : 0x4000<=val<0x5000
    OK_BEGIN_RESERVED=0x4000,
    OK_CANCEL_CHECKOUT_checkout_empty,
    OK_CANCEL_CHECKOUT_checkout_cancelled,
    OK_GET_TICKET_LIST,
    OK_START_CHECKOUT,
    OK_BUY,
    OK_VERSION_VALIDATION,
    OK_END_RESERVED
};

static_assert((quint16)ServerMessageType::ERROR_END_RESERVED<0x1000);
static_assert((quint16)ServerMessageType::CRIT_SERVER_ERR_END_RESERVED<0x2000);
static_assert((quint16)ServerMessageType::CRIT_CLIENT_ERR_END_RESERVED<0x3000);
static_assert((quint16)ServerMessageType::CLIENT_WARNING_END_RESERVED<0x4000);
static_assert((quint16)ServerMessageType::OK_END_RESERVED<0x5000);

enum class ClientMessageType:quint16{
    Invalid=0x0000,
    REQUEST_BEGIN_RESERVED,
    REQUEST_VERSION_VALIDATION,
    REQUEST_GET_TICKET_LIST,
    REQUEST_START_CHECKOUT,
    REQUEST_BUY,
    REQUEST_CANCEL_CHECKOUT,
    REQUEST_END_RESERVED,
};

constexpr quint16 CATEGORY_MASK = 0xF000;
enum class ServerMessageCategory : quint16 {
    Error         = 0x0000,
    ServerCrit    = 0x1000,
    ClientCrit    = 0x2000,
    ClientWarning = 0x3000,
    Ok            = 0x4000
};

constexpr bool isServerMessageTypeReserved(ServerMessageType type)noexcept{
    switch(type){
        case ServerMessageType::ERROR_BEGIN_RESERVED:
        case ServerMessageType::CRIT_SERVER_ERR_BEGIN_RESERVED:
        case ServerMessageType::CRIT_CLIENT_ERR_BEGIN_RESERVED:
        case ServerMessageType::CLIENT_WARNING_BEGIN_RESERVED:
        case ServerMessageType::OK_BEGIN_RESERVED:
        case ServerMessageType::ERROR_END_RESERVED:
        case ServerMessageType::CRIT_SERVER_ERR_END_RESERVED:
        case ServerMessageType::CRIT_CLIENT_ERR_END_RESERVED:
        case ServerMessageType::CLIENT_WARNING_END_RESERVED:
        case ServerMessageType::OK_END_RESERVED:
            return true;
        default:
            return false;
    }
}

constexpr bool serverMessageCheckCategory(ServerMessageType type,ServerMessageCategory category)noexcept{
    if(isServerMessageTypeReserved(type))return false;
    return (static_cast<quint16>(type) & CATEGORY_MASK) == static_cast<quint16>(category);
}
struct ServerMessage{
    ServerMessageType type;
    QByteArray message="";//optional
};
struct ClientMessage{
    ClientMessageType type;
    QByteArray message="";//optional
};
namespace parsing{
    //Does not guarantee that unpacked message will be valid
    ClientMessage unpackClientMessage(const QByteArray& data){
        if(data.size()<2)throw std::invalid_argument("Not_enough_bytes");
        return{
            static_cast<ClientMessageType>(parsing::unpackNumber<quint16>(data)),
            data.mid(2)
        };
    }

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

    template<typename T>
    requires std::is_integral_v<T> && std::is_unsigned_v<T>
    T unpackNumber(const QByteArray& array,quint32 index=0){
        quint32 size=static_cast<quint32>(array.size());
        if(index>size || index+sizeof(T)>size)throw std::out_of_range("Not_enough_bits_left");
        return qFromBigEndian<T>(reinterpret_cast<const uchar*>(array.constData()+index));
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
    QHash<QByteArray,Ticket> tickets;
    QHash<ClientSession*,QByteArray> inCheckout;
public:
    TicketStore(){
        //pull tickets from database
        //log and rejects invalid ticket lengths or other issues
        //placeholder:
        tickets.emplace("Lorem_Ipsum",Ticket{123,10});
        tickets.emplace("Dolor_Sit_Amet",Ticket{111,0});

        if(tickets.size()>255)qFatal("Failed to start the server: Loaded too many tickets from the database");
        if(tickets.size()>10)qWarning()<<"SERVER_WARNING: unusually large amount of tickets loaded";
        if(tickets.empty())qFatal("Failed to start the server: Failed to load tickets from the database");
    }

    ServerMessage getTicketList(){
        if(tickets.empty())return{ServerMessageType::CRIT_SERVER_ERR_Ticket_list_empty};
        
        //todo: Subscribe socket to ticket availability updates.

        QByteArray answer;
        
        if(tickets.size()>255)return {ServerMessageType::CRIT_SERVER_ERR_Ticket_list_overflow};
        quint8 validTickets=0;
        //reserve space for validTickets;
        answer.append('\0');

        for(auto iterator=tickets.cbegin();iterator!=tickets.cend();iterator++){
            const QByteArray& name=iterator.key();
            const Ticket& data=iterator.value();

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
        if(validTickets==0)return {ServerMessageType::CRIT_SERVER_ERR_No_valid_tickets_found};
        answer.data()[0]=validTickets;
        return {ServerMessageType::OK_GET_TICKET_LIST,answer};
    }

    ServerMessage tryCancelCheckout(ClientSession* socket,bool disconnectCleanup=false){
        //todo: connect so it runs 5 minutes after successful tryCheckout and sends such information through the socket, unless called or canceled by buy.

        auto currentCheckout=inCheckout.find(socket);
        if(currentCheckout==inCheckout.end()){
            if(disconnectCleanup)return {ServerMessageType::OK_CANCEL_CHECKOUT_checkout_empty};
            else return {ServerMessageType::ERR_No_checkout_in_progress};
        }
        auto currentTicket = tickets.find(currentCheckout.value());
        if(currentTicket==tickets.end()){
            inCheckout.erase(currentCheckout);
            return {ServerMessageType::CRIT_CLIENT_ERR_Invalid_ticket_in_reservation};
        }

        currentTicket.value().availableAmount++;
        inCheckout.erase(currentCheckout);
        return {ServerMessageType::OK_CANCEL_CHECKOUT_checkout_cancelled};
    }

    ServerMessage tryCheckout(ClientSession* socket,const ClientMessage& request){

        if(request.message.isEmpty())return {ServerMessageType::ERR_Checkout_with_no_arguments};
        
        QByteArray ticketName;
        try{
            quint32 offset=0;
            ticketName=parsing::unpack8BitPrefixedByteArray(request.message,offset);
            if(offset!=request.message.size())return {ServerMessageType::ERR_Parsing_error};
        }catch(std::exception& e){
            return {ServerMessageType::ERR_Parsing_error};
        }
        if(ticketName.isEmpty())return {ServerMessageType::ERR_Ticket_name_empty};


        auto checkout = inCheckout.find(socket);
        if(checkout!=inCheckout.end()){
            if(checkout.value()==ticketName)return {ServerMessageType::CLIENT_WARNING_Ticket_already_in_checkout};
            else return {ServerMessageType::ERR_Different_ticket_already_in_checkout};
        }
        auto ticket = tickets.find(ticketName);
        if(ticket==tickets.end())return {ServerMessageType::ERR_Invalid_ticket_name};
        
        if(ticket.value().availableAmount>0){
            inCheckout.emplace(socket,ticketName);
            ticket.value().availableAmount--;
            return {ServerMessageType::OK_START_CHECKOUT,ticketName};
        }else{
            return {ServerMessageType::ERR_No_tickets_in_stock_during_checkout};
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

    ServerMessage confirmPurchase(ClientSession* socket,const ClientMessage& request){

        QByteArray buyerName;
        QByteArray ticketName;
        try{
            quint32 offset=0;
            buyerName=parsing::unpack8BitPrefixedByteArray(request.message,offset);
            ticketName=parsing::unpack8BitPrefixedByteArray(request.message,offset);
            if(offset!=request.message.size())return {ServerMessageType::ERR_Parsing_error};
        }catch(std::exception& e){
            return {ServerMessageType::ERR_Parsing_error};
        }
        if(buyerName.isEmpty() || ticketName.isEmpty())return {ServerMessageType::ERR_Empty_string_argument};

        auto reservation = inCheckout.find(socket);
        //keep item in checkout unless error specifies otherwise
        if(validateName(buyerName)==false)return {ServerMessageType::ERR_Disallowed_name_try_again};
        if(reservation==inCheckout.end())return {ServerMessageType::ERR_Item_not_in_checkout};
        if(reservation.value()!=ticketName)return {ServerMessageType::ERR_Wrong_item_in_checkout};
        if(tickets.find(ticketName)==tickets.end()){
            inCheckout.erase(reservation);
            return {ServerMessageType::ERR_Ticket_no_longer_valid};
        }
        //todo: stop the 5 minutes cancel-checkout clock
        //todo: Push [name][ticket_name] into the database. On fail return error. For now as a placeholder:
        qInfo()<<buyerName+" purchased ticket for "+ticketName;
        inCheckout.erase(reservation);
        
        return {ServerMessageType::OK_BUY,ticketName};
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

    ServerMessage validateClientVersion(const ClientMessage& request){
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
                return {ServerMessageType::OK_VERSION_VALIDATION};
            }
            else{
                return {ServerMessageType::CRIT_CLIENT_ERR_Version_mismatch};
            }
        }
    }

    ServerMessage craftResponse(const ClientMessage& request){

        if(request.type==ClientMessageType::REQUEST_VERSION_VALIDATION){
            return validateClientVersion(request);
        }

        if(versionValidated==false){
            return {ServerMessageType::ERR_Version_not_validated};
        }

        if(request.type==ClientMessageType::REQUEST_GET_TICKET_LIST){
            if(request.message.isEmpty()==false)return {ServerMessageType::ERR_Incorrect_argument_count};
            else return store->getTicketList();
        }
        //Format: START_CHECKOUT <ticket_name>
        if(request.type==ClientMessageType::REQUEST_START_CHECKOUT){
            return store->tryCheckout(this,request);
        }
        //format: BUY <customer_name> <ticket_name>
        if(request.type==ClientMessageType::REQUEST_BUY){
            return store->confirmPurchase(this,request);
        }
        if(request.type==ClientMessageType::REQUEST_CANCEL_CHECKOUT){
            if(request.message.isEmpty()==false)return {ServerMessageType::ERR_Incorrect_argument_count};
            else return store->tryCancelCheckout(this);
        }
        return {ServerMessageType::ERR_Unknown_Command};
    }

    QByteArray frameResponse(const ServerMessage& response){

        QByteArray answer;
        quint32 size=response.message.size()+sizeof(response.type);
        answer.reserve(response.message.size()+sizeof(response.type)+sizeof(size));
        
        answer.append(parsing::packNumber(size));
        answer.append(parsing::packNumber((quint16)response.type));
        answer.append(response.message);
        return answer;
    }

    QByteArray buildDiagnostic(const QByteArray& prefix,const ServerMessage& error,const ClientMessage& request){
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

    [[noreturn]] void fatalShutDown(const ServerMessage& error,const ClientMessage& request){
        QByteArray errorString=buildDiagnostic("CRIT_SERVER_ERROR",error,request);
        //todo: dump all possibly relevant information into a file
        //todo: send general information about shutdown to all connected sockets,then try to shutdown gracefully. 
        //placeholder:
        qFatal("%s", errorString.constData());
    }

    
    void fail(const ServerMessage& error,const ClientMessage& request){
        QByteArray errorString=buildDiagnostic("ERROR",error,request);
        qWarning()<<errorString;
        socket->write(frameResponse(error));
        socket->disconnectFromHost();
    }

    void onReadyRead() {
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