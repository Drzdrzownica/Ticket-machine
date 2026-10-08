#include "client/UI/MainPage.h"

void MainPage::disableInput(){
    debugEditCoinsBtn->setDisabled(true);
    languagesBtn->setDisabled(true);
    purchaseBtn->setDisabled(true);
    exitBtn->setDisabled(true);
}

void MainPage::reEnableInput(){
    debugEditCoinsBtn->setDisabled(false);
    languagesBtn->setDisabled(false);
    purchaseBtn->setDisabled(false);
    exitBtn->setDisabled(false);
}

MainPage::MainPage():layout(this){

    debugEditCoinsBtn = new QPushButton("DEBUG edit coins contents",this);
    languagesBtn = new QPushButton("Languages",this);
    purchaseBtn = new QPushButton("Buy ticket",this);
    exitBtn = new QPushButton("EXIT",this);


    layout.addWidget(debugEditCoinsBtn);
    layout.addWidget(languagesBtn);
    layout.addWidget(purchaseBtn);
    layout.addWidget(exitBtn);
    connect(debugEditCoinsBtn,&QPushButton::clicked,this,[this](){disableInput(); emit debugEditCoinsOption();});
    connect(languagesBtn,&QPushButton::clicked,this,[this](){disableInput(); emit languagesOption();});
    connect(purchaseBtn,&QPushButton::clicked,this,[this](){disableInput(); emit purchaseOption();});
    //it's fine here to hard quit because in real hardware this button would not exist
    connect(exitBtn,&QPushButton::clicked,&QApplication::quit);
}
void MainPage::reinitialize(Language language){
    reEnableInput();
}