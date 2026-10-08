#include "client/UI/LoginPage.h"

void LoginPage::disableInput(){
    inputBox->setDisabled(true);
    loginButton->setDisabled(true);
}
void LoginPage::reEnableInput(){
    inputBox->setDisabled(false);
    loginButton->setDisabled(false);
}

void LoginPage::onLoginClicked(){
    disableInput();
    QString dbId=inputBox->text();
    inputBox->setText("");
    emit dataBaseIDProvided(dbId);
}

LoginPage::LoginPage():layout(this){
    instruction=new QLabel("Awaiting server connection. Please wait.",this);
    inputBox=new QLineEdit(this);
    tryAgainMessage=new QLabel("No DB with given ID. Try again",this);
    loginButton=new QPushButton("login",this);
    layout.addWidget(instruction);
    layout.addWidget(inputBox);
    layout.addWidget(tryAgainMessage);
    layout.addWidget(loginButton);
    disableInput();
    tryAgainMessage->setVisible(false);
    connect(loginButton,&QPushButton::clicked,this,&LoginPage::onLoginClicked);
    connect(inputBox,&QLineEdit::returnPressed,this,&LoginPage::onLoginClicked);
}
void LoginPage::dbIdRejected(){
    tryAgainMessage->setVisible(true);
    reEnableInput();
    inputBox->setFocus();
}
void LoginPage::allowLogin(){
    instruction->setText("Enter The DB id \n(the local databases are not yet hooked up. For now enter \"temp\")");
    reEnableInput();
}