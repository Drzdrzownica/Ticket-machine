#ifndef PROTOCOL_MESSAGES_H
#define PROTOCOL_MESSAGES_H
#include <QCoreApplication>

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
    OK,
    OK_No_change,
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

struct ServerResponse{
    ServerMessageType type;
    ClientMessageType inResponseTo;
    QByteArray message="";
    bool operator==(const ServerResponse&)const=default;
};
#endif