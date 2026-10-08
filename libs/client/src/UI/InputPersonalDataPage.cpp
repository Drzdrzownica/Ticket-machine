#include "client/UI/InputPersonalDataPage.h"

void InputPersonalDataPage::disableInput(){
    confirmButton->setDisabled(true);
    inputField->setDisabled(true);
    cancelButton->setDisabled(true);
    disconnect(inputSubmitConnection);
}

void InputPersonalDataPage::reEnableInput(){
    confirmButton->setDisabled(false);
    inputField->setDisabled(false);
    cancelButton->setDisabled(false);
    if(!inputSubmitConnection)inputSubmitConnection = connect(inputField,&QLineEdit::returnPressed,this,&InputPersonalDataPage::onSubmitt);
}

void InputPersonalDataPage::onSubmitt(){
    disableInput();
    emit personalDataSubmitted(inputField->text());
    inputField->setText("");
}

InputPersonalDataPage::InputPersonalDataPage():layout(this){
    layout.addWidget(title);
    layout.addWidget(inputField);
    layout.addWidget(errorMessage);
    errorMessage->setVisible(false);
    layout.addWidget(confirmButton);
    layout.addWidget(cancelButton);
    connect(cancelButton,&QPushButton::clicked,this,[this](){
        disableInput();
        inputField->setText("");
        emit cancelPressed();
    }
    );
    inputSubmitConnection = connect(inputField,&QLineEdit::returnPressed,this,&InputPersonalDataPage::onSubmitt);
    connect(confirmButton,&QPushButton::clicked,this,&InputPersonalDataPage::onSubmitt);
}
void InputPersonalDataPage::reinitialize(const TicketData& data,Language language){
    reEnableInput();
    errorMessage->setVisible(false);
    title->setText("Input name associated with the ticket for "+ data.name);
    inputField->setFocus();
}
void InputPersonalDataPage::submittedNameNotAccepted(){
    reEnableInput();
    errorMessage->setVisible(true);
}