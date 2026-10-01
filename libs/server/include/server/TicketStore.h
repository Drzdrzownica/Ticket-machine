#ifndef TICKET_STORE_H
#define TICKET_STORE_H
#pragma once
#include "protocol/serialization.h"
#include "protocol/constants.h"

using SessionId = quint64;

class TicketStore{
    struct Ticket{
        QByteArray name;
        Cents cost;
        quint32 availableAmount;
        Ticket(QByteArray name,Cents cost,quint32 amount):name(name),cost(cost),availableAmount(amount){};
        Ticket()=default;
    };
    QHash<TicketId,Ticket> tickets;
    QHash<SessionId,TicketId> inCheckout;
public:
    TicketStore();

    ServerMessage getTicketList();

    ServerMessage tryCancelCheckout(SessionId sessionId,bool disconnectCleanup=false);

    ServerMessage tryCheckout(SessionId sessionId,const ClientMessage& request);

    bool validateName(const QByteArray& buyerName);

    ServerMessage handleNameValidationRequest(const ClientMessage& request);

    ServerMessage confirmPurchase(SessionId sessionId,const ClientMessage& request);
};
#endif //TICKET_STORE_H
