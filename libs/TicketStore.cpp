#include "TicketStore.h"

TicketStore::TicketStore(){
    //pull tickets from database
    //log and rejects invalid ticket lengths or other issues
    //placeholder:
    tickets.emplace("Lorem_Ipsum",Ticket{123,10});
    tickets.emplace("Dolor_Sit_Amet",Ticket{111,0});

    if(tickets.size()>255)qFatal("Failed to start the server: Loaded too many tickets from the database");
    if(tickets.size()>10)qWarning()<<"SERVER_WARNING: unusually large amount of tickets loaded";
    if(tickets.isEmpty())qFatal("Failed to start the server: Failed to load tickets from the database");
}

ServerMessage TicketStore::getTicketList(){
    if(tickets.isEmpty())return{ServerMessageType::CRIT_SERVER_ERR_Ticket_list_empty};
    
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
    return {ServerMessageType::OK,answer};
}

ServerMessage TicketStore::tryCancelCheckout(quint64 id,bool disconnectCleanup){
    //todo: connect so it runs 5 minutes after successful tryCheckout and sends such information through the socket, unless called or canceled by buy.

    auto currentCheckout=inCheckout.find(id);
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

ServerMessage TicketStore::tryCheckout(quint64 id,const ClientMessage& request){
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


    auto checkout = inCheckout.find(id);
    if(checkout!=inCheckout.end()){
        if(checkout.value()==ticketName)return {ServerMessageType::CLIENT_WARNING_Ticket_already_in_checkout};
        else return {ServerMessageType::ERR_Different_ticket_already_in_checkout};
    }
    auto ticket = tickets.find(ticketName);
    if(ticket==tickets.end())return {ServerMessageType::ERR_Invalid_ticket_name};
    
    if(ticket.value().availableAmount>0){
        inCheckout.emplace(id,ticketName);
        ticket.value().availableAmount--;
        return {ServerMessageType::OK,ticketName};
    }else{
        return {ServerMessageType::ERR_No_tickets_in_stock_during_checkout};
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

ServerMessage TicketStore::confirmPurchase(quint64 id,const ClientMessage& request){

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

    auto reservation = inCheckout.find(id);

    if(reservation==inCheckout.end())return {ServerMessageType::ERR_Item_not_in_checkout};
    if(tickets.find(ticketName)==tickets.end()){
        inCheckout.erase(reservation);
        return {ServerMessageType::ERR_Ticket_no_longer_valid};
    }
    if(reservation.value()!=ticketName)return {ServerMessageType::ERR_Wrong_item_in_checkout};
    if(validateName(buyerName)==false)return {ServerMessageType::ERR_Disallowed_name_try_again};
    //todo: stop the 5 minutes cancel-checkout clock
    //todo: Push [name][ticket_name] into the database. On fail return error. For now as a placeholder:
    qInfo()<<buyerName+" purchased ticket for "+ticketName;
    inCheckout.erase(reservation);
    
    return {ServerMessageType::OK,ticketName};
}