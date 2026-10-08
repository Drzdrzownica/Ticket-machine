#ifndef RETURNINGMONEYPAGE_H
#define RETURNINGMONEYPAGE_H

#include <QVBoxLayout>
#include <QLabel>
#include "client/ClientTypes.h"

class ReturningMoneyPage:public QWidget{
    Q_OBJECT
    QVBoxLayout layout;

    QLabel* reasonMessage=new QLabel;
    QLabel* pleaseWaitMessage=new QLabel("Returning inserted coins. Please wait.");
public:
    ReturningMoneyPage();
    void reinitialize(TransactionCancellationReason reason,Language language);
};
#endif