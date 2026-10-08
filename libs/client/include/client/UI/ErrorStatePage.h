#ifndef ERRORSTATEPAGE_H
#define ERRORSTATEPAGE_H

#include <QVBoxLayout>
#include <QLabel>
#include <QWidget>
#include "client/ClientTypes.h"

class ErrorStatePage: public QWidget{
    Q_OBJECT
    QVBoxLayout layout;
    QLabel* information;
    QLabel* details;
public:
    ErrorStatePage();
    void reinitialize(ClientShutdownReason reason,Language language);
};
#endif // ERRORSTATEPAGE_H
