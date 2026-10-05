#ifndef CONSTANTS_H
#define CONSTANTS_H

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
using Cents = quint64;
using PacketLengthPrefix = quint32;
inline const QByteArray protocolVersion = "0.0.0.1";

#endif //DEBUG_LOG