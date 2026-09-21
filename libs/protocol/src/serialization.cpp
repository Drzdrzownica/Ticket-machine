#include "protocol/serialization.h"

QByteArray parsing::pack8BitPrefixedByteArray(const QByteArray& array){
    QByteArray result;
    if(array.size()>255)throw std::invalid_argument("array_too_long");
    result.append(static_cast<quint8>(array.size()));
    result.append(array);
    return result;
}

QByteArray parsing::unpack8BitPrefixedByteArray(const QByteArray& parameters,qsizetype& offset){
    QByteArray answer;
    if(parameters.size()<=offset)throw std::out_of_range("Offset_larger_than_parameters_size");
    quint8 len=parameters[offset++];
    if(len>parameters.size()-offset)throw std::out_of_range("Prefix_larger_than_remaining_message");
    QByteArray result = parameters.mid(offset,len);
    offset+=len;
    return result;
}

ClientMessage parsing::unpackClientMessage(const QByteArray& rawData){
    qsizetype offset=0;
    ClientMessageType clientMessage = parsing::unpackEnum<ClientMessageType>(rawData,offset);
    QByteArray data=rawData.mid(offset);
    return{clientMessage,data};
}

ServerResponse parsing::unpackServerResponse(const QByteArray& rawData){
    qsizetype offset=0;
    ServerMessageType message=parsing::unpackEnum<ServerMessageType>(rawData,offset);
    ClientMessageType inResponseTo = parsing::unpackEnum<ClientMessageType>(rawData,offset);
    QByteArray data=rawData.mid(offset);
    return {message,inResponseTo,data};
}