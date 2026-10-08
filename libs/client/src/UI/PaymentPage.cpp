#include "client/UI/PaymentPage.h"

void PaymentPage::disableInput(){
    confirmButton->setDisabled(true);
    coinSlot->setDisabled(true);
    cancelButton->setDisabled(true);
    insertButton->setDisabled(true);
    disconnect(insertSubmitConnection);
}

void PaymentPage::reEnableInput(){
    if(isEnough)confirmButton->setDisabled(false);
    coinSlot->setDisabled(false);
    cancelButton->setDisabled(false);
    insertButton->setDisabled(false);
    if(!insertSubmitConnection)insertSubmitConnection= connect(coinSlot,&QLineEdit::returnPressed,this,&PaymentPage::processCoin);
}

void PaymentPage::processCoin(){
    disableInput();
    emit denominationInserted(coinSlot->text());
    coinSlot->setText("");
}   

PaymentPage::PaymentPage():layout(this){
    layout.addWidget(instruction);
    layout.addWidget(coinSlot);
    coinSlot->setPlaceholderText("Enter value in cents representing a coin/bill");
    layout.addWidget(unknownCoinMessage);
    unknownCoinMessage->setVisible(false);
    layout.addWidget(insertButton);
    layout.addWidget(insertedMessage);
    layout.addWidget(confirmButton);
    confirmButton->setDisabled(true);
    layout.addWidget(cancelButton);
    connect(cancelButton,&QPushButton::clicked,this,[this](){disableInput(); emit cancelPressed();});
    connect(insertButton,&QPushButton::clicked,this,&PaymentPage::processCoin);
    insertSubmitConnection= connect(coinSlot,&QLineEdit::returnPressed,this,&PaymentPage::processCoin);
    connect(confirmButton,&QPushButton::clicked,this,[this](){disableInput(); emit confirmPressed();});
}
void PaymentPage::reinitialize(const TicketData& data,Language language){
    isEnough=false;
    instruction->setText("insert "+centsToPriceString(data.price)+" in coins or bills");
    reEnableInput();
    insertedMessage->setText("so far inserted $0.00");
    coinSlot->setFocus();
}
void PaymentPage::amountInsertedChanged(Cents insertedAmount,bool isEnough){
    insertedMessage->setText("so far inserted "+centsToPriceString(insertedAmount));
    this->isEnough=isEnough;
    reEnableInput();
    unknownCoinMessage->setVisible(false);
    coinSlot->setFocus();
}
void PaymentPage::unknownCoin(){
    reEnableInput();
    unknownCoinMessage->setVisible(true);
    coinSlot->setFocus();
}