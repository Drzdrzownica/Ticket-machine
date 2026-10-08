#ifndef CHOOSETICKETPAGE_H
#define CHOOSETICKETPAGE_H

#include <QVBoxLayout>
#include <QLabel>
#include <vector>
#include <QPushButton>
#include "client/ClientTypes.h"

class ChooseTicketPage: public QWidget{
    Q_OBJECT
    QVBoxLayout layout;
    QVBoxLayout* buttonsLayout;
    
    QLabel* errorMessage;
    QPushButton* backButton;
    std::vector<QPushButton*> ticketButtons;

    void disableInput();
    void reEnableInput();
    void clearTicketButtons();
public:
    ChooseTicketPage();
    void reinitialize(const std::vector<TicketData>& listOfTickets,Language language);
signals:
    void backPressed();
    void ticketPicked(TicketData);
};
#endif //CHOOSETICKETPAGE_H