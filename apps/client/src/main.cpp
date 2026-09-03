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
        //it's fine here to hard quit because in real hardware this button would not exist
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
    QPushButton* backButton=new QPushButton("Back",this);

public:
    LanguagesPage():layout(this){
        layout.addWidget(englishBtn);
        layout.addWidget(backButton);
        connect(englishBtn,&QPushButton::clicked,this,[this](){emit languagePicked(Language::English);});
        connect(backButton,&QPushButton::clicked,this,&LanguagesPage::backPressed);
    }
signals:
    void backPressed();
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
    void reinitialize(const std::vector<TicketData>& listOfTickets){
        clearTicketButtons();
        if(listOfTickets.empty()){
            errorMessage->setVisible(true);
        }else{
            errorMessage->setVisible(false);
            for(const auto& ticket:listOfTickets){
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

    QLabel* title=new QLabel(this);
    QLineEdit* inputField=new QLineEdit(this) ;
    QLabel* errorMessage=new QLabel("The name can not be empty",this);
    QPushButton* confirmButton = new QPushButton("confirm",this);
    QPushButton* cancelButton= new QPushButton("Cancel",this);

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
            emit personalDataSubmitted(inputField->text());
            inputField->setText("");
        }
        );
    }
    void reinitialize(const TicketData& data){
        errorMessage->setVisible(false);
        title->setText("Input name associated with the ticket for "+ data.name);
    }
signals:
    void cancelPressed();
    void personalDataSubmitted(QString name);
public slots:
    //later we might pass some reason, but for now we assume it's because it was empty
    void submittedNameNotAccepted(){
        errorMessage->setVisible(true);
    }
};

class LoginPage: public QWidget{
    Q_OBJECT
    QVBoxLayout layout;
    QLabel* instruction=new QLabel("Enter The DB id",this);
    QLineEdit* inputBox=new QLineEdit(this);
    QLabel* tryAgainMessage=new QLabel("No DB with given ID. Try again",this);
    QPushButton* loginButton=new QPushButton("login",this);

    void loginClicked(){
        inputBox->setDisabled(true);
        loginButton->setDisabled(true);
        QString dbId=inputBox->text();
        inputBox->setText("");
        emit dataBaseIDProvided(dbId);
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
public slots:
    void dbIdRejected(){
        tryAgainMessage->setVisible(true);
        inputBox->setDisabled(false);
        loginButton->setDisabled(false);
    }
signals:
    void dataBaseIDProvided(QString);
};

class PaymentPage: public QWidget{
    Q_OBJECT
    QVBoxLayout layout;
    

    QLabel* instruction=new QLabel("You are not supposed to see this message",this);
    QLineEdit* coinSlot=new QLineEdit(this);
    QPushButton* insertButton=new QPushButton("insert",this);
    QLabel* unknownCoinMessage=new QLabel("The coin was rejected",this);
    QLabel* insertedMessage=new QLabel("You are not supposed to see this message",this);
    QPushButton* confirmButton= new QPushButton("confirm",this);
    QPushButton* cancelButton=new QPushButton("Cancel",this);
    
    void processCoin(){
        emit denominationInserted(coinSlot->text());
        coinSlot->setText("");
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
        connect(confirmButton,&QPushButton::clicked,this,&PaymentPage::confirmPressed);
    }
    void reinitialize(const TicketData& data){
        instruction->setText("insert "+centsToPriceString(data.price)+" in coins or bills");
        confirmButton->setDisabled(true);
        insertedMessage->setText("so far inserted $0.00");
    }
signals:
    void cancelPressed();

    void denominationInserted(QString);
    void confirmPressed();

public slots:
    void amountInsertedChanged(Cents insertedAmount,bool isEnough){
        insertedMessage->setText("so far inserted "+centsToPriceString(insertedAmount));
        unknownCoinMessage->setVisible(false);
        if(isEnough)confirmButton->setDisabled(false);
    }
    void unknownCoin(){
        unknownCoinMessage->setVisible(true);
    }

};

class PrintingPage:public QWidget{
    Q_OBJECT
    QVBoxLayout layout;
    QLabel* message=new QLabel("You are not supposed to see this message",this);

public:
    PrintingPage():layout(this){
        layout.addWidget(message);
    }

    //This is obviously an abstraction of a physical process. Real implementation wouldn't need timers. But it will need data to know what we're printing. Printing introduces edgecases, but we don't worry about that now.
    void startPrinting(){
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


class PaymentProcessor:public QObject{
Q_OBJECT
Cents amountCurrentlyInserted=0;
Cents ticketCost=0;

public:
PaymentProcessor(QObject* parent=nullptr):QObject(parent){}

using placeholderChangeType = int;//will make it a real one later
std::optional<placeholderChangeType> canGiveOutChange(){
    return 0;
}

signals:

void amountInsertedChanged(Cents newValue,bool isEnough);
void invalidCoinInserted();
void purchaseCompleted();

public slots:

    void insertCoin(Cents coinVal){
        amountCurrentlyInserted+=coinVal;
        if(coinVal!=0)emit amountInsertedChanged(amountCurrentlyInserted,amountCurrentlyInserted>=ticketCost); //there are no 0-cent coins so the check is unnecesary but it's there for compleatness
    }

    //placeholder. For now accept blindly
    void tryAcceptPurchase(){
        //if can give out change

        if(amountCurrentlyInserted<ticketCost){
            //emit ...
        }else{
            auto change=canGiveOutChange();
            if(!change){
                //emit ...
            }else{
                //give out change
                amountCurrentlyInserted=0;
                emit purchaseCompleted();
            }
        }
    }

    void cancelTransaction(){
        //return coins
        amountCurrentlyInserted=0;

    }

    void startTransaction(Cents cost){
        if(amountCurrentlyInserted!=0);//hande error. todo
        amountCurrentlyInserted=0;
        ticketCost=cost;
    }
};

class SessionController:public QObject{
Q_OBJECT
    PlaceholderForLocalData localData;
    Language language=Language::English;    
    std::vector<TicketData> availableTickets;
    TicketData currentTicket;
    PaymentProcessor* paymentProcessor;

    //placeholder, I will later connect it to the backend. Ultimately we want to be getting updates asynchronously.
    std::vector<TicketData> retrieveTicketListFromServer(){
        std::vector<TicketData> result;
        result.push_back({"lorem",1});
        result.push_back({"ipsum",2});
        return result;
    }

    std::optional<Cents> identifyCoin(const QString& coin){
        if(coin=="1")return 1;
        if(coin=="5")return 5;
        if(coin=="10")return 10;
        if(coin=="25")return 25;
        if(coin=="100")return 100;
        if(coin=="500")return 500;
        if(coin=="1000")return 1000;
        if(coin=="2000")return 2000;
        return std::nullopt;
    }

    //Ultimatly this will be a server call, so for now this is a bare-bones placeholder.
    bool verifyName(QString name){
        if(name.isEmpty())return false;
        return true;
    }

public:

    const std::vector<TicketData>& getAvailableTickets() const {
        return availableTickets;
    }

    Language getLanguage() const {
        return language;
    }
    
    SessionController(QObject* parent=nullptr):QObject(parent){
        paymentProcessor= new PaymentProcessor(this);
        connect(paymentProcessor,&PaymentProcessor::purchaseCompleted,this,&SessionController::purchaseCompleted);
        connect(paymentProcessor,&PaymentProcessor::amountInsertedChanged,this,&SessionController::amountInsertedChanged);
    }

public slots:

    //for now placeholder, later try to log into the local db and retrieve real data. /
    //The DB is just local storage, but since this is just a simulation of a real machine, /
    //there can be multiple machines with multiple DBs simulated on a single PC so we have to differentiate them
    void dbIdProvided(QString dbId){
        if(dbId=="temp"){
            localData = PlaceholderForLocalData{};
            availableTickets=retrieveTicketListFromServer();
            emit sessionReady();
        }else{
            emit unknownDBId();
        }
    }

    void languageChanged(Language lang){
        language=lang;
        emit sessionReady();
    }

    void logPrinted(){
        //todo
        emit sessionReady();
    }

    void cancelTransaction(){
        paymentProcessor->cancelTransaction();
        emit sessionReady();

    }

    void tryCoin(QString coin){
        auto coinVal=identifyCoin(coin);
        if(!coinVal)emit invalidCoinInserted();
        else{
            paymentProcessor->insertCoin(coinVal.value());
        }
    }

    void tryAcceptPurchase(){
        paymentProcessor->tryAcceptPurchase();
    }

    void startTransaction(){
        paymentProcessor->startTransaction(currentTicket.price);
    }

    void ticketPicked(TicketData data){
        currentTicket=data;
        //todo check if ticket is still valid
        emit ticketChoiceAccepted(currentTicket);
    }

    void validateBuyerName(QString name){
        if(verifyName(name)==false){
            emit nameRejected();
        }else{
            emit nameAccepted(currentTicket);
        }
    }

signals:
    void unknownDBId();
    void nameAccepted(TicketData);
    void nameRejected();
    void amountInsertedChanged(Cents,bool isEnough);
    void sessionReady();
    void invalidCoinInserted();
    void purchaseCompleted();
    void ticketChoiceAccepted(TicketData);
};

class MainWindow:public QWidget{
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

    SessionController* session;


public:
    MainWindow(QWidget* parent=nullptr):QWidget(parent),layout(this){
        
        session=new SessionController(this);
        connect(session,&SessionController::sessionReady,this,&MainWindow::goToMain);

        //stack.addWidget() passes ownership to the stack immidietly after the 'new'
        loginPage=new LoginPage;
        stack.addWidget(loginPage);
        connect(loginPage,&LoginPage::dataBaseIDProvided,session,&SessionController::dbIdProvided);
        connect(session,&SessionController::unknownDBId,loginPage,&LoginPage::dbIdRejected);


        mainPage=new MainPage;
        stack.addWidget(mainPage);
        connect(mainPage,&MainPage::languagesOption,this,&MainWindow::goToLanguages);
        connect(mainPage,&MainPage::purchaseOption,this,&MainWindow::goToChooseTicketPage);
        

        languagesPage=new LanguagesPage;
        stack.addWidget(languagesPage);
        connect(languagesPage,&LanguagesPage::languagePicked,session,&SessionController::languageChanged);
        connect(languagesPage,&LanguagesPage::backPressed,this,&MainWindow::goToMain);


        chooseTicketPage=new ChooseTicketPage();
        stack.addWidget(chooseTicketPage);
        connect(chooseTicketPage,&ChooseTicketPage::backPressed,this,&MainWindow::goToMain);
        connect(chooseTicketPage,&ChooseTicketPage::ticketPicked,session,&SessionController::ticketPicked);
        connect(session,&SessionController::ticketChoiceAccepted,this,&MainWindow::goToInputPersonalDataPage);


        inputPersonalDataPage=new InputPersonalDataPage();
        stack.addWidget(inputPersonalDataPage);
        connect(inputPersonalDataPage,&InputPersonalDataPage::cancelPressed,this,&MainWindow::goToMain);
        connect(inputPersonalDataPage,&InputPersonalDataPage::personalDataSubmitted,session,&SessionController::validateBuyerName);
        connect(session,&SessionController::nameRejected,inputPersonalDataPage,&InputPersonalDataPage::submittedNameNotAccepted);
        connect(session,&SessionController::nameAccepted,this,&MainWindow::goToTakeCoinsPage);


        paymentPage=new PaymentPage();
        stack.addWidget(paymentPage);
        connect(paymentPage,&PaymentPage::cancelPressed,session,&SessionController::cancelTransaction);
        connect(paymentPage,&PaymentPage::denominationInserted,session,&SessionController::tryCoin);
        connect(paymentPage,&PaymentPage::confirmPressed,session,&SessionController::tryAcceptPurchase);
        connect(session,&SessionController::invalidCoinInserted,paymentPage,&PaymentPage::unknownCoin);
        connect(session,&SessionController::amountInsertedChanged,paymentPage,&PaymentPage::amountInsertedChanged);
        connect(session,&SessionController::purchaseCompleted,this,&MainWindow::goToPrintingPage);


        printingPage=new PrintingPage;
        stack.addWidget(printingPage);
        connect(printingPage,&PrintingPage::printingFinished,session,&SessionController::logPrinted);


        stack.setCurrentWidget(loginPage);
        layout.addWidget(&stack);
    }
public slots:

    //conceptually we'll call reinitialize on all of them, while also passing the current language, but now it's not necessary.
    void goToMain(){
        stack.setCurrentWidget(mainPage);
    }

    void goToLanguages(){
        stack.setCurrentWidget(languagesPage);
    }

    void goToPrintingPage(){
        printingPage->startPrinting();
        stack.setCurrentWidget(printingPage);
    }

    void goToTakeCoinsPage(const TicketData& data){
        paymentPage->reinitialize(data);
        session->startTransaction();
        stack.setCurrentWidget(paymentPage);
    }

    void goToInputPersonalDataPage(const TicketData& data){
        inputPersonalDataPage->reinitialize(data);
        stack.setCurrentWidget(inputPersonalDataPage);
    }

    void goToChooseTicketPage(){
        chooseTicketPage->reinitialize(session->getAvailableTickets());
        stack.setCurrentWidget(chooseTicketPage);
    }

};

int main(int argc, char *argv[]){
    QApplication app(argc, argv);
    MainWindow window;
    window.show();

    return app.exec();
}

#include "main.moc"
