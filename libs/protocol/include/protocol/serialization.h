#ifndef SERIALIZATION_H
#define SERIALIZATION_H
#include "protocol/messages.h"
#include <QtEndian>

namespace parsing{
    QByteArray pack8BitPrefixedByteArray(const QByteArray& array);
    QByteArray unpack8BitPrefixedByteArray(const QByteArray& parameters,quint32& offset);
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

    //Does not guarantee that unpacked message will be valid
    ClientMessage unpackClientMessage(const QByteArray& data);
}
#endif