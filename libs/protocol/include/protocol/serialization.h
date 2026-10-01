#ifndef SERIALIZATION_H
#define SERIALIZATION_H
#include "protocol/messages.h"
#include <QtEndian>


namespace parsing{
    QByteArray pack8BitPrefixedByteArray(const QByteArray& array);
    QByteArray unpack8BitPrefixedByteArray(const QByteArray& parameters,qsizetype& offset);
    template<typename T> requires std::is_integral_v<T> && std::is_unsigned_v<T> QByteArray packNumber(T number){
        number = qToBigEndian(number);
        QByteArray result;
        result.append(reinterpret_cast<const char*>(&number),sizeof(number));
        return result; 
    }

    template<typename T> requires std::is_integral_v<T> && std::is_unsigned_v<T> T unpackNumber(const QByteArray& array,qsizetype& offset){
        qsizetype originalOffset=offset;
        qsizetype size=array.size();
        //the first condition is redundant in most cases, but it's here as an overflow protection, mainly for a principle.
        if(originalOffset>size || originalOffset+sizeof(T)>size)throw std::out_of_range("Not_enough_bits_left");
        T result=qFromBigEndian<T>(reinterpret_cast<const uchar*>(array.constData()+originalOffset));
        offset+=sizeof(T);
        return result;
    }
    template<typename T> requires std::is_integral_v<T> && std::is_unsigned_v<T> T unpackNumber(const QByteArray& array){
        qsizetype noOffset=0;
        return unpackNumber<T>(array,noOffset);
    }
    template<typename E> requires std::is_enum_v<E> E unpackEnum(const QByteArray& array,qsizetype& offset){
        using U = std::underlying_type_t<E>;
        return static_cast<E>(unpackNumber<U>(array, offset));
    }
    template<typename E> requires std::is_enum_v<E> E unpackEnum(const QByteArray& array){
        qsizetype noOffset=0;
        return unpackEnum<E>(array,noOffset);
    }
    
    //Do not guarantee that unpacked message will be valid
    ClientMessage unpackClientMessage(const QByteArray& data);
    ServerResponse unpackServerResponse(const QByteArray& data);
}
#endif //SERIALIZATION_H