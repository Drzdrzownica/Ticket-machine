#include "protocol/serialization.h"

QByteArray parsing::pack8BitPrefixedByteArray(const QByteArray& array){
    QByteArray result;
    if(array.size()>255)throw std::invalid_argument("array_too_long");
    result.append(static_cast<quint8>(array.size()));
    result.append(array);
    return result;
}

QByteArray parsing::unpack8BitPrefixedByteArray(const QByteArray& parameters,quint32& offset){
    QByteArray answer;
    if(parameters.size()<=offset)throw std::out_of_range("Offset_larger_than_parameters_size");
    quint8 len=parameters[offset++];
    if(len>parameters.size()-offset)throw std::out_of_range("Prefix_larger_than_remaining_message");
    QByteArray result = parameters.mid(offset,len);
    offset+=len;
    return result;
}

ClientMessage parsing::unpackClientMessage(const QByteArray& data){
    if(data.size()<2)throw std::invalid_argument("Not_enough_bytes");
    return{
        static_cast<ClientMessageType>(parsing::unpackNumber<quint16>(data)),
        data.mid(2)
    };
}

ServerResponse parsing::unpackServerResponse(const QByteArray& data){
    if(data.size()<4)throw std::invalid_argument("Not_enough_bytes");
    return{
        static_cast<ServerMessageType>(parsing::unpackNumber<quint16>(data)),
        static_cast<ClientMessageType>(parsing::unpackNumber<quint16>(data.mid(2))),
        data.mid(4)
    };
}