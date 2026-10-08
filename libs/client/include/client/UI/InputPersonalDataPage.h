#ifndef INPUTPERSONALDATAPAGE_H
#define INPUTPERSONALDATAPAGE_H

#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMetaObject>
#include "client/ClientTypes.h"
class InputPersonalDataPage:public QWidget{
    Q_OBJECT
    QVBoxLayout layout;

    QLabel* title=new QLabel(this);
    QLineEdit* inputField=new QLineEdit(this) ;
    QLabel* errorMessage=new QLabel("The name you provided has been rejected. Try again.",this);
    QPushButton* confirmButton = new QPushButton("confirm",this);
    QPushButton* cancelButton= new QPushButton("Cancel",this);
    QMetaObject::Connection inputSubmitConnection;
    void disableInput();
    void reEnableInput();
    void onSubmitt();

public:
    InputPersonalDataPage();
    void reinitialize(const TicketData& data,Language language);
signals:
    void cancelPressed();
    void personalDataSubmitted(QString name);
public slots:
    void submittedNameNotAccepted();
};
#endif //INPUTPERSONALDATAPAGE_H