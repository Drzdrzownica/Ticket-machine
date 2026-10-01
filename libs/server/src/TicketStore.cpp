#include "server/TicketStore.h"

TicketStore::TicketStore(){
    //pull tickets from database
    //log and rejects invalid ticket lengths or other issues
    //placeholder:
    tickets.emplace(0,Ticket{"Lorem",123,10});
    tickets.emplace(1,Ticket{"Ipsum",200,10});
    tickets.emplace(2,Ticket{"Dolor",113,2});
    tickets.emplace(3,Ticket{"Sit",499,50});
    tickets.emplace(4,Ticket{"Amet",111,0});

    if(tickets.size()>255)qFatal("Failed to start the server: Loaded too many tickets from the database");
    if(tickets.size()>10)qWarning()<<"SERVER_WARNING: unusually large amount of tickets loaded";
    if(tickets.isEmpty())qFatal("Failed to start the server: Failed to load tickets from the database");
}

ServerMessage TicketStore::getTicketList(){
    if(tickets.isEmpty())return{ServerMessageType::CRIT_SERVER_ERR_Ticket_list_empty};
    
    //todo: Subscribe socket to ticket availability updates.

    QByteArray answer;
    
    if(tickets.size()>255)return {ServerMessageType::CRIT_SERVER_ERR_Ticket_list_overflow};
    quint8 numberOfValidTickets=0;
    //reserve space for numberOfValidTickets;
    answer.append('\0');

    for(auto iterator=tickets.cbegin();iterator!=tickets.cend();iterator++){
        const TicketId& ticketId=iterator.key();
        const Ticket& data=iterator.value();

        QByteArray ticketPacketData;
        try{
            ticketPacketData.append(parsing::packNumber(ticketId));
            ticketPacketData.append(parsing::pack8BitPrefixedByteArray(data.name));
            ticketPacketData.append(parsing::packNumber(data.cost));
            //only if it's available - yes/no.
            ticketPacketData.append((data.availableAmount>0) ? 1 : 0);
        }catch(std::exception& e){
            qWarning()<<"SERVER_ERROR failed_to_process_ticket:"<<e.what();
            continue;
        }
        numberOfValidTickets++;
        answer.append(ticketPacketData);
    }
    if(numberOfValidTickets==0)return {ServerMessageType::CRIT_SERVER_ERR_No_valid_tickets_found};
    answer.data()[0]=numberOfValidTickets;
    return {ServerMessageType::OK,answer};
}

ServerMessage TicketStore::tryCancelCheckout(SessionId sessionId,bool disconnectCleanup){
    //todo: connect so it runs 5 minutes after successful tryCheckout and sends such information through the socket, unless called or canceled by buy.

    auto currentCheckout=inCheckout.find(sessionId);
    if(currentCheckout==inCheckout.end()){
        if(disconnectCleanup)return {ServerMessageType::OK_No_change};
        else return {ServerMessageType::ERR_No_checkout_in_progress};
    }
    auto currentTicket = tickets.find(currentCheckout.value());
    if(currentTicket==tickets.end()){
        inCheckout.erase(currentCheckout);
        return {ServerMessageType::CRIT_CLIENT_ERR_Invalid_ticket_in_reservation};
    }

    currentTicket.value().availableAmount++;
    inCheckout.erase(currentCheckout);
    return {ServerMessageType::OK};
}

ServerMessage TicketStore::tryCheckout(SessionId sessionId,const ClientMessage& request){
    if(request.message.isEmpty())return {ServerMessageType::ERR_Checkout_with_no_arguments};
    
    TicketId ticketId;
    try{
        qsizetype offset=0;
        ticketId=parsing::unpackNumber<TicketId>(request.message,offset);
        if(offset!=request.message.size())return {ServerMessageType::CRIT_CLIENT_ERR_Parsing_error};
    }catch(std::exception& e){
        return {ServerMessageType::CRIT_CLIENT_ERR_Parsing_error};
    }

    auto checkout = inCheckout.find(sessionId);
    if(checkout!=inCheckout.end()){
        if(checkout.value()==ticketId)return {ServerMessageType::CLIENT_WARNING_Ticket_already_in_checkout};
        else return {ServerMessageType::ERR_Different_ticket_already_in_checkout};
    }
    auto ticket = tickets.find(ticketId);
    if(ticket==tickets.end())return {ServerMessageType::ERR_Invalid_ticket_id};
    
    if(ticket.value().availableAmount>0){
        inCheckout.emplace(sessionId,ticketId);
        ticket.value().availableAmount--;
        return {ServerMessageType::OK,parsing::packNumber(ticketId)};
    }else{
        return {ServerMessageType::ERR_No_tickets_in_stock_while_initiating_checkout};
    }
}

//For now as a placeholder rule only allow ascii letters and no spaces or special characters.
bool TicketStore::validateName(const QByteArray& buyerName){
    if(buyerName.isEmpty())return false;
    for(auto character:buyerName){
        if(character>='a' && character<='z')continue;
        if(character>='A' && character<='Z')continue;
        return false;
    }
    return true;
}

ServerMessage TicketStore::handleNameValidationRequest(const ClientMessage& request){
    QByteArray buyerName;
    try{
        qsizetype offset=0;
        buyerName=parsing::unpack8BitPrefixedByteArray(request.message,offset);
        if(offset!=request.message.size())return {ServerMessageType::CRIT_CLIENT_ERR_Parsing_error};
    }catch(std::exception& e){
        return {ServerMessageType::CRIT_CLIENT_ERR_Parsing_error};
    }
    if(validateName(buyerName))return {ServerMessageType::OK,parsing::packNumber<quint8>(true)};
    else return {ServerMessageType::OK,parsing::packNumber<quint8>(false)};
}

ServerMessage TicketStore::confirmPurchase(SessionId sessionId,const ClientMessage& request){

    QByteArray buyerName;
    TicketId ticketId;
    try{
        qsizetype offset=0;
        buyerName=parsing::unpack8BitPrefixedByteArray(request.message,offset);
        ticketId=parsing::unpackNumber<TicketId>(request.message,offset);
        if(offset!=request.message.size())return {ServerMessageType::CRIT_CLIENT_ERR_Parsing_error};
    }catch(std::exception& e){
        return {ServerMessageType::CRIT_CLIENT_ERR_Parsing_error};
    }
    auto reservation = inCheckout.find(sessionId);
    
    if(reservation==inCheckout.end())return {ServerMessageType::ERR_Item_not_in_checkout};
    
    if(reservation.value()!=ticketId){
        inCheckout.erase(reservation);
        return {ServerMessageType::ERR_Wrong_item_in_checkout};
    }
    inCheckout.erase(reservation);

    if(tickets.find(ticketId)==tickets.end())return {ServerMessageType::ERR_Ticket_no_longer_valid};
    if(validateName(buyerName)==false)return {ServerMessageType::ERR_Disallowed_name};

    //todo: stop the 5 minutes cancel-checkout clock
    //todo: Push [name][ticket_ID] into the database. On fail return error. For now as a placeholder:
    qInfo()<<buyerName+" purchased ticket for id"<<ticketId;
    
    return {ServerMessageType::OK,parsing::packNumber<TicketId>(ticketId)};
}