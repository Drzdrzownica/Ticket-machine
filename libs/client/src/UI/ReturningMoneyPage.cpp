
#include "client/UI/ReturningMoneyPage.h"

ReturningMoneyPage::ReturningMoneyPage():layout(this){
    layout.addWidget(reasonMessage);
    layout.addWidget(pleaseWaitMessage);
}
void ReturningMoneyPage::reinitialize(TransactionCancellationReason reason,Language language){
    reasonMessage->setVisible(true);
    switch (reason){
    case TransactionCancellationReason::CouldNotGiveOutChange:
        reasonMessage->setText("Sorry, the machine could not produce the exact change");
        break;
    case TransactionCancellationReason::UserRequested:
        reasonMessage->setVisible(false);
        break;
    case TransactionCancellationReason::Error: //continue to the default case
    default:
        reasonMessage->setText("Sorry, something went wrong. Returning inserted money");
        break;
    }
}