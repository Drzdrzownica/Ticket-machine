#ifndef SERIALIZATION_H
#define SERIALIZATION_H
#include "protocol/messages.h"
#include <QtEndian>

namespace parsing{
    QByteArray pack8BitPrefixedByteArray(const QByteArray& array);
    QByteArray unpack8BitPrefixedByteArray(const QByteArray& parameters,quint32& offset);
    template<typename T> requires std::is_integral_v<T> && std::is_unsigned_v<T> QByteArray packNumber(T number){
        number = qToBigEndian(number);
        QByteArray result;
        result.append(reinterpret_cast<const char*>(&number),sizeof(number));
        return result; 
    }

    template<typename T> requires std::is_integral_v<T> && std::is_unsigned_v<T> T unpackNumber(const QByteArray& array,quint32& offset){
        quint32 originalOffset=offset;
        quint32 size=static_cast<quint32>(array.size());
        //the first condition is redundant in most cases, but it's here as an overflow protection, mainly for a principle.
        if(originalOffset>size || originalOffset+sizeof(T)>size)throw std::out_of_range("Not_enough_bits_left");
        T result=qFromBigEndian<T>(reinterpret_cast<const uchar*>(array.constData()+originalOffset));
        offset+=sizeof(T);
        return result;
    }
    template<typename T> requires std::is_integral_v<T> && std::is_unsigned_v<T> T unpackNumber(const QByteArray& array){
        quint32 noOffset=0;
        return unpackNumber<T>(array,noOffset);
    }

    //Do not guarantee that unpacked message will be valid
    ClientMessage unpackClientMessage(const QByteArray& data);
    ServerResponse unpackServerResponse(const QByteArray& data);
}
#endif