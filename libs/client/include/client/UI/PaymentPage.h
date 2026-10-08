#ifndef PATMENTPAGE_H
#define PATMENTPAGE_H

#include "client/ClientTypes.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMetaObject>

class PaymentPage: public QWidget{
    Q_OBJECT
    QVBoxLayout layout;
    
    QLabel* instruction=new QLabel("You are not supposed to see this message",this);
    QLineEdit* coinSlot=new QLineEdit(this);
    QPushButton* insertButton=new QPushButton("insert",this);
    QMetaObject::Connection insertSubmitConnection;
    QLabel* unknownCoinMessage=new QLabel("The coin was rejected",this);
    QLabel* insertedMessage=new QLabel("You are not supposed to see this message",this);
    QPushButton* confirmButton= new QPushButton("confirm",this);
    QPushButton* cancelButton=new QPushButton("Cancel",this);
    
    bool isEnough=false;

    void disableInput();
    void reEnableInput();
    void processCoin();
public:
    PaymentPage();
    void reinitialize(const TicketData& data,Language language);
signals:
    void cancelPressed();
    void denominationInserted(QString);
    void confirmPressed();
public slots:
    void amountInsertedChanged(Cents insertedAmount,bool isEnough);
    void unknownCoin();
};
#endif //PATMENTPAGE_H