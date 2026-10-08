#include "client/UI/LanguagesPage.h"

void LanguagesPage::disableInput(){
    englishBtn->setDisabled(true);
    backButton->setDisabled(true);
}

void LanguagesPage::reEnableInput(){
    englishBtn->setDisabled(false);
    backButton->setDisabled(false);
}

LanguagesPage::LanguagesPage():layout(this){
    englishBtn=new QPushButton("English "+QString::fromUcs4(U"\U0001F1FA\U0001F1F8"),this);
    backButton=new QPushButton("Back",this);
    layout.addWidget(englishBtn);
    layout.addWidget(backButton);
    connect(englishBtn,&QPushButton::clicked,this,[this](){disableInput(); emit languagePicked(Language::English);});
    connect(backButton,&QPushButton::clicked,this,[this](){disableInput(); emit backPressed();});
}
void LanguagesPage::reinitialize(Language language){
    reEnableInput();
}