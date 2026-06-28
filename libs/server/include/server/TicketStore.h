#ifndef TICKET_STORE_H
#define TICKET_STORE_H
#pragma once
#include "protocol/serialization.h"

class TicketStore{
    struct Ticket{
        quint32 cost;
        quint32 availableAmount;
        Ticket(quint32 cost,quint32 amount):cost(cost),availableAmount(amount){};
        Ticket()=default;
    };
    QHash<QByteArray,Ticket> tickets;
    QHash<quint64,QByteArray> inCheckout;
public:
    TicketStore();

    ServerMessage getTicketList();

    ServerMessage tryCancelCheckout(quint64 id,bool disconnectCleanup=false);

    ServerMessage tryCheckout(quint64 id,const ClientMessage& request);

    bool validateName(const QByteArray& buyerName);

    ServerMessage confirmPurchase(quint64 id,const ClientMessage& request);
};
#endif
