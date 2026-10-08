
#include "client/UI/ErrorTryAgainPage.h"

void ErrorTryAgainPage::disableInput(){
    okButton->setDisabled(true);
}
void ErrorTryAgainPage::reEnableInput(){
    okButton->setDisabled(false);
}

ErrorTryAgainPage::ErrorTryAgainPage():layout(this){
    errorMessage=new QLabel("Something went wrong. Please try again.",this);
    okButton=new QPushButton("OK",this);
    layout.addWidget(errorMessage);
    layout.addWidget(okButton);
    connect(okButton,&QPushButton::clicked,this,[this](){disableInput();emit okClicked();});
}
void ErrorTryAgainPage::reinitialize(Language language){
    reEnableInput();
}