#include <QCoreApplication>
#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QLineEdit>
#include <iostream>
#include <QTimer>
#include <QObject>


//this is meant as client-side UI, meant to run on PC and simulate a version that will run on dedicated hardware,
using Cents = quint32;
QString centsToPriceString(Cents cents){
    return QString("$%1.%2").arg(cents/100).arg(cents%100,2,10,QChar('0'));
}
//just to look pretty, I won't bother implementing languages, at least I don't think I will.
enum class Language{
    English
};

//stuff like, what coins the machine contains (if it can output exact change), etc. This and all other structs below are essentially placeholders for now.
struct PlaceholderForLocalData{
    int dummyVal=0;
};

struct TicketData{
    QString name;
    Cents price;
};

struct FullPurchaseData{
    TicketData ticketData;
    QString buyerName;
};



class MainPage:public QWidget{
    Q_OBJECT
    QVBoxLayout layout;

    //languages are a non-functional placeholde (since a page with a single option seems redundant,and I still want to have a main page)
    QPushButton* languagesBtn =new QPushButton("Languages",this);
    QPushButton* purchaseBtn = new QPushButton("Buy ticket",this);
    QPushButton* exitBtn=new QPushButton("EXIT",this);

public:
    MainPage():layout(this){
        layout.addWidget(languagesBtn);
        layout.addWidget(purchaseBtn);
        layout.addWidget(exitBtn);
        connect(languagesBtn,&QPushButton::clicked,this,&MainPage::languagesOption);
        connect(purchaseBtn,&QPushButton::clicked,this,&MainPage::purchaseOption);
        connect(exitBtn,&QPushButton::clicked,&QApplication::quit);
    }
signals:
    void languagesOption();
    void purchaseOption();
};


class LanguagesPage:public QWidget{
    Q_OBJECT
    QVBoxLayout layout;

    QPushButton* englishBtn=new QPushButton("English "+QString::fromUcs4(U"\U0001F1FA\U0001F1F8"),this);

public:
    LanguagesPage():layout(this){
        layout.addWidget(englishBtn);
        connect(englishBtn,&QPushButton::clicked,this,[this](){emit languagePicked(Language::English);});
    }
signals:
    void languagePicked(Language);
};

class ChooseTicketPage: public QWidget{
    Q_OBJECT
    QVBoxLayout layout;
    QVBoxLayout* buttonsLayout;
    
    QLabel* errorMessage=new QLabel("Sorry, no tickets available");
    QPushButton* backButton=new QPushButton("Back",this);

    std::vector<QPushButton*> ticketButtons;

    void clearTicketButtons(){
        for(auto* btn:ticketButtons){
            buttonsLayout->removeWidget(btn);
            btn->deleteLater();
        }
        ticketButtons.clear();
    }

public:
    ChooseTicketPage():layout(this){
        buttonsLayout=new QVBoxLayout;
        layout.addLayout(buttonsLayout);
        layout.addWidget(errorMessage);
        layout.addWidget(backButton);
        connect(backButton,&QPushButton::clicked,this,&ChooseTicketPage::backPressed);
    }
    void reinitialize(const std::vector<TicketData>& data){
        clearTicketButtons();
        if(data.empty()){
            errorMessage->setVisible(true);
        }else{
            errorMessage->setVisible(false);
            for(const auto& ticket:data){
                QPushButton* btn=new QPushButton(ticket.name,this);
                ticketButtons.push_back(btn);
                buttonsLayout->addWidget(btn);
                connect(btn,&QPushButton::clicked,this,[this,ticket](){
                    emit ticketPicked(ticket);
                });
            }
        }
    }
signals:
    void backPressed();
    void ticketPicked(TicketData);
};

class InputPersonalDataPage:public QWidget{
    Q_OBJECT
    QVBoxLayout layout;

    TicketData ticketData;

    QLabel* title=new QLabel(this);
    QLineEdit* inputField=new QLineEdit(this) ;
    QLabel* errorMessage=new QLabel("The name can not be empty",this);
    QPushButton* confirmButton = new QPushButton("confirm",this);
    QPushButton* cancelButton= new QPushButton("Cancel",this);


    //Ultimatly the possible input will be restricted by the keyboard in the machine so user won't be able to use any unicode shenanigans
    bool verifyName(){
        if(inputField->text().isEmpty())return false;
        return true;
    }

    void processSubmission(){
        if(verifyName()==false){
            inputField->clear();
            errorMessage->setVisible(true);
        }else{
            FullPurchaseData data{ticketData,inputField->text()};
            emit personalDataConfirmed(data);
        }
    }
public:
    InputPersonalDataPage():layout(this){
        layout.addWidget(title);
        layout.addWidget(inputField);
        layout.addWidget(errorMessage);
        errorMessage->setVisible(false);
        layout.addWidget(confirmButton);
        layout.addWidget(cancelButton);
        connect(cancelButton,&QPushButton::clicked,this,[this](){
            inputField->setText("");
            emit cancelPressed();
        }
        );
        connect(confirmButton,&QPushButton::clicked,this,[this](){
            processSubmission();
            inputField->setText("");
        }
        );
    }
    void reinitialize(const TicketData& data){
        errorMessage->setVisible(false);
        ticketData=data;
        title->setText("Input name associated with the ticket for "+ data.name);
    }
signals:
    void cancelPressed();
    void personalDataConfirmed(FullPurchaseData);
};

class LoginPage: public QWidget{
    Q_OBJECT
    QVBoxLayout layout;
    QLabel* instruction=new QLabel("Enter The DB id",this);
    QLineEdit* inputBox=new QLineEdit(this);
    QLabel* tryAgainMessage=new QLabel("No DB with given ID. Try again",this);
    QPushButton* loginButton=new QPushButton("login",this);

    void loginClicked(){
        //for now placeholder, later try to log into the local db and retrieve real data. /
        //The DB is just local storage, but since this is just a simulation of a real machine, /
        //there can be multiple machines with multiple DBs simulated on a single PC so we have to diferentiate them
        if(inputBox->text()=="temp"){
            PlaceholderForLocalData temp=PlaceholderForLocalData{};
            tryAgainMessage->setVisible(false);
            emit loggedIntoDatabase(temp);
        }else{
            tryAgainMessage->setVisible(true);
        }
    }

public:
    LoginPage():layout(this){
        layout.addWidget(instruction);
        layout.addWidget(inputBox);
        layout.addWidget(tryAgainMessage);
        layout.addWidget(loginButton);
        tryAgainMessage->setVisible(false);
        connect(loginButton,&QPushButton::clicked,this,&LoginPage::loginClicked);
    }
signals:
    void loggedIntoDatabase(PlaceholderForLocalData);
};

class PaymentPage: public QWidget{
    Q_OBJECT
    QVBoxLayout layout;
    
    FullPurchaseData data;

    QLabel* instruction=new QLabel("You are not supposed to see this message",this);
    QLineEdit* coinSlot=new QLineEdit(this);
    QPushButton* insertButton=new QPushButton("insert",this);
    QLabel* unknownCoinMessage=new QLabel("The coin was rejected",this);
    QLabel* insertedMessage=new QLabel("You are not supposed to see this message",this);
    QPushButton* confirmButton= new QPushButton("confirm",this);
    QPushButton* cancelButton=new QPushButton("Cancel",this);
    
    Cents insertedAmount=0; 
    
    //placeholder. For now accept blindly
    void tryAcceptPurchase(){
        //if can give out change
        if(true){
            //give out change
            emit paymentCompleted(data);
        }else{
            //go to refusal page and then to main menu
        }
    }

    //placehilder
    bool canGiveOutChange(){
        return true;
    }

    std::optional<Cents> tryCoin(){
        if(coinSlot->text()=="1")return 1;
        if(coinSlot->text()=="5")return 5;
        if(coinSlot->text()=="10")return 10;
        if(coinSlot->text()=="25")return 25;
        if(coinSlot->text()=="100")return 100;
        if(coinSlot->text()=="500")return 500;
        if(coinSlot->text()=="1000")return 1000;
        if(coinSlot->text()=="2000")return 2000;
        return std::nullopt;
    }
    
    void processCoin(){
        std::optional<Cents> coin=tryCoin();
        coinSlot->setText("");
        if(!coin){
            unknownCoinMessage->setVisible(true);
            return;
        }else unknownCoinMessage->setVisible(false);
        insertedAmount+=coin.value();
        insertedMessage->setText("so far inserted "+centsToPriceString(insertedAmount));
        if(insertedAmount>=data.ticketData.price)confirmButton->setDisabled(false);
    }



public:
    PaymentPage():layout(this){
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
        connect(cancelButton,&QPushButton::clicked,this,&PaymentPage::cancelPressed);
        connect(insertButton,&QPushButton::clicked,this,&PaymentPage::processCoin);
        connect(confirmButton,&QPushButton::clicked,this,&PaymentPage::tryAcceptPurchase);
    }
    void reinitialize(const FullPurchaseData& data){
        this->data=data;
        instruction->setText("insert "+centsToPriceString(data.ticketData.price)+" in coins or bills");
        insertedAmount=0;
        confirmButton->setDisabled(true);
        insertedMessage->setText("so far inserted "+centsToPriceString(insertedAmount));
    }
signals:
    void cancelPressed();
    void paymentCompleted(FullPurchaseData);
};

class PrintingPage:public QWidget{
    Q_OBJECT
    QVBoxLayout layout;
    QLabel* message=new QLabel("You are not supposed to see this message",this);

public:
    PrintingPage():layout(this){
        layout.addWidget(message);
    }

    //This is obviously an abstarction of a physical process. Real implementation wouldn't need timers. But it will need data to know what we're printing. Printing introduces edgecases, but we don't worry about that now.
    void startPrinting(const FullPurchaseData& data){
        message->setText("Printing in progress...");
        QTimer::singleShot(3000, this, [this] {
            message->setText("Printing Done");
            QTimer::singleShot(2000, this, [this] {
                emit printingFinished();
            });
        });
    }
signals:
    void printingFinished();
};



class MainWindowController:public QWidget{
    Q_OBJECT

    QVBoxLayout layout;
    QStackedWidget stack;

    MainPage* mainPage;
    LoginPage* loginPage;
    LanguagesPage* languagesPage;
    ChooseTicketPage* chooseTicketPage;
    InputPersonalDataPage* inputPersonalDataPage;
    PaymentPage* paymentPage;
    PrintingPage* printingPage;

    PlaceholderForLocalData localData;
    Language language=Language::English;
    std::vector<TicketData> availableTickets;


    void completeLoginRetrieveData(const PlaceholderForLocalData& data){
        localData=data;
        availableTickets=retrieveTicketListFromServer();
        goToMain();
    }

    //placeholder, I will later connect it to the backend. Ultimatly we want to be getting updates asynchronously.
    std::vector<TicketData> retrieveTicketListFromServer(){
        std::vector<TicketData> result;
        result.push_back({"lorem",1});
        result.push_back({"ipsum",2});
        return result;
    }

    void goToMain(){
        stack.setCurrentWidget(mainPage);
    }

    void goToLanguages(){
        stack.setCurrentWidget(languagesPage);
    }

    void receiveLanguageChange(Language lang){
        language=lang;
        goToMain();
    }

    void goToPrintingPage(const FullPurchaseData& data){
        printingPage->startPrinting(data);
        stack.setCurrentWidget(printingPage);
    }

    void goToTakeCoinsPage(const FullPurchaseData& data){
        paymentPage->reinitialize(data);
        stack.setCurrentWidget(paymentPage);
    }

    void goToInputPersonalDataPage(const TicketData& data){
        inputPersonalDataPage->reinitialize(data);
        stack.setCurrentWidget(inputPersonalDataPage);
    }

    void generateAndGoToChooseTicketPage(){
        chooseTicketPage->reinitialize(availableTickets);
        stack.setCurrentWidget(chooseTicketPage);
    }

public:
    MainWindowController():layout(this){
        
        //stack.addWidget() immidietly pass ownership to the stack
        loginPage=new LoginPage;
        stack.addWidget(loginPage);
        connect(loginPage,&LoginPage::loggedIntoDatabase,this,&MainWindowController::completeLoginRetrieveData);
        
        mainPage=new MainPage;
        stack.addWidget(mainPage);
        connect(mainPage,&MainPage::languagesOption,this,&MainWindowController::goToLanguages);
        connect(mainPage,&MainPage::purchaseOption,this,&MainWindowController::generateAndGoToChooseTicketPage);
        
        languagesPage=new LanguagesPage;
        stack.addWidget(languagesPage);
        connect(languagesPage,&LanguagesPage::languagePicked,this,&MainWindowController::receiveLanguageChange);

        printingPage=new PrintingPage;
        stack.addWidget(printingPage);
        connect(printingPage,&PrintingPage::printingFinished,this,&MainWindowController::goToMain);

        paymentPage=new PaymentPage();
        stack.addWidget(paymentPage);
        connect(paymentPage,&PaymentPage::cancelPressed,this,&MainWindowController::goToMain);
        connect(paymentPage,&PaymentPage::paymentCompleted,this,&MainWindowController::goToPrintingPage);

        inputPersonalDataPage=new InputPersonalDataPage();
        stack.addWidget(inputPersonalDataPage);
        connect(inputPersonalDataPage,&InputPersonalDataPage::cancelPressed,this,&MainWindowController::goToMain);
        connect(inputPersonalDataPage,&InputPersonalDataPage::personalDataConfirmed,this,&MainWindowController::goToTakeCoinsPage);

        chooseTicketPage=new ChooseTicketPage();
        connect(chooseTicketPage,&ChooseTicketPage::backPressed,this,&MainWindowController::goToMain);
        connect(chooseTicketPage,&ChooseTicketPage::ticketPicked,this,&MainWindowController::goToInputPersonalDataPage);
        stack.addWidget(chooseTicketPage);

        stack.setCurrentWidget(loginPage);
        layout.addWidget(&stack);
    }
};

int main(int argc, char *argv[]){
    QApplication app(argc, argv);
    MainWindowController window;
    window.show();

    return app.exec();
}

#include "main.moc"
