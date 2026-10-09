#ifndef CLIENTTYPES_H
#define CLIENTTYPES_H

#include "protocol/constants.h"

//just to look pretty, I won't bother implementing languages, at least I don't think I will.
enum class Language{
    English
};

enum class TransactionCancellationReason{
    CouldNotGiveOutChange,
    ServerDisconnected,
    UserRequested,
    SocketError,
    BuyFailed,
    Error
};

enum class ClientShutdownReason:quint64{
    Version_validation_mismatch,
    Version_validation_issue,
    Ticket_list_parsing_issue,
    Ticket_list_retrieval_issue,
    Generic_server_requested,
    Response_parsing_issue,
    Unexpected_response_type,
    Transaction_state_violation,
    Server_disconnected,
    Socket_error,
    CoinInventory_Logic_error
};

struct TicketData{
    TicketId Id; //alias of an integral type defined in constants.h for compatibility with server
    QString name;
    Cents price; //same as above
    bool isAvailable=true;
};

#endif //CLIENTTYPES_H