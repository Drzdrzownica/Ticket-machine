
#include "client/UI/ChooseTicketPage.h"

void ChooseTicketPage::disableInput(){
    backButton->setDisabled(true);
    for(auto& btn:ticketButtons){
        btn->setDisabled(true);
    }
}

void ChooseTicketPage::reEnableInput(){
    backButton->setDisabled(false);
    for(auto& btn:ticketButtons){
        btn->setDisabled(false);
    }
}

void ChooseTicketPage::clearTicketButtons(){
    for(auto* btn:ticketButtons){
        buttonsLayout->removeWidget(btn);
        btn->deleteLater();
    }
    ticketButtons.clear();
}
ChooseTicketPage::ChooseTicketPage():layout(this){
    errorMessage=new QLabel("Sorry, no tickets available");
    backButton=new QPushButton("Back",this);
    buttonsLayout=new QVBoxLayout;
    layout.addLayout(buttonsLayout);
    layout.addWidget(errorMessage);
    layout.addWidget(backButton);
    connect(backButton,&QPushButton::clicked,this,[this](){disableInput(); emit backPressed();});
}
void ChooseTicketPage::reinitialize(const std::vector<TicketData>& listOfTickets,Language language){
    clearTicketButtons();
    reEnableInput();
    if(listOfTickets.empty()){
        errorMessage->setVisible(true);
    }else{
        errorMessage->setVisible(false);
        for(const auto& ticket:listOfTickets){
            QString buttonText=ticket.name;
            if(ticket.isAvailable==false)buttonText+=" - SOLD OUT";
            else buttonText+=" - "+centsToPriceString(ticket.price);
            QPushButton* btn=new QPushButton(buttonText,this);
            btn->setEnabled(ticket.isAvailable);
            ticketButtons.push_back(btn);
            buttonsLayout->addWidget(btn);
            connect(btn,&QPushButton::clicked,this,[this,ticket](){
                disableInput();
                emit ticketPicked(ticket);
            });
        }
    }
}