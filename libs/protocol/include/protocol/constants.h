#ifndef CONSTANTS_H
#define CONSTANTS_H
#pragma once
#include <QtGlobal>
#include <QString>
#include <QByteArray>

//#define DEBUG_LOG

#ifdef DEBUG_LOG
#ifdef _MSC_VER
#pragma message("WARNING: Logging potentially sensitive data is enabled")
#else
#warning "WARNING: Logging potentially sensitive data is enabled"
#endif

#endif

#if defined(DEBUG_LOG) &&  defined(NDEBUG)
#error "DEBUG_LOG cannot be enabled in release builds"
#endif

using TicketId = quint64;
using PacketLengthPrefix = quint32;
inline const QByteArray protocolVersion = "0.0.0.1";
using Cents = quint64;

inline QString centsToPriceString(Cents cents){
    return QString("$%1.%2").arg(cents/100).arg(cents%100,2,10,QChar('0'));
}

#endif //CONSTANTS_H