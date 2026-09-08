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

using Cents = quint64;

struct CoinInventory{
    static constexpr std::array<Cents,8> acceptedDenominationsCents{1,5,10,25,100,500,1000,2000};
    static bool isValidDenomination(Cents value){
        return std::find(acceptedDenominationsCents.begin(),acceptedDenominationsCents.end(),value)!=acceptedDenominationsCents.end();
    }
    static bool isValidDenomination(const QString& str){
        bool isNum;
        quint64 value=str.toULongLong(&isNum);
        if(!isNum)return false;
        return isValidDenomination(value);
    }
private:
    std::array<quint64,acceptedDenominationsCents.size()> inventory{};
    quint64& amountOf(Cents value){
        //the competetive programmer living in my heart screams at me to optimize those O(n) look-ups, but it's really unnecessary here.
        auto iterator=std::find(acceptedDenominationsCents.begin(),acceptedDenominationsCents.end(),value);
        Q_ASSERT(iterator!=acceptedDenominationsCents.end());
        return inventory[std::distance(acceptedDenominationsCents.begin(),iterator)];
    }
public:
    void addCoin(Cents value,quint64 amount=1){
        amountOf(value)+=amount;
    }
    void subtractCoin(Cents value,quint64 amount=1){
        Q_ASSERT(amountOf(value)>=amount);
        amountOf(value)-=amount;
    }

    std::vector<std::pair<Cents,quint64>> getInventoryList()const{
        std::vector<std::pair<Cents,quint64>>result;
        for(std::size_t i=0;i<inventory.size();i++){
            result.push_back({acceptedDenominationsCents[i],inventory[i]});
        }
        return result;
    }
    //we explicitly don't worry about overflow here. There is litterally not enough money in the world
    Cents getTotalAmount() const{
        Cents result=0;
        for(std::size_t i=0;i<inventory.size();i++){
            result+=inventory[i]*acceptedDenominationsCents[i];
        }
        return result;
    }

    auto operator<=>(const CoinInventory&)const=default;
    void moveInventoryFrom(CoinInventory& other){
        Q_ASSERT(this!=&other);
        for(std::size_t i=0;i<inventory.size();i++){
            inventory[i]+=other.inventory[i];
            other.inventory[i]=0;
        }
    }
    void subtractInventory(const CoinInventory& other){
        for(std::size_t i=0;i<inventory.size();i++){
            Q_ASSERT(inventory[i]>=other.inventory[i]);
            inventory[i]-=other.inventory[i];
        }
    }
    void addInventory(const CoinInventory& other){
        for(std::size_t i=0;i<inventory.size();i++){
            inventory[i]+=other.inventory[i];
        }
    }

    friend CoinInventory operator+(const CoinInventory& a,const CoinInventory& b){
        CoinInventory result{};
        result.addInventory(a);
        result.addInventory(b);
        return result;
    }

};

//this is meant as client-side UI, meant to run on PC and simulate a version that will run on dedicated hardware,
QString centsToPriceString(Cents cents){
    return QString("$%1.%2").arg(cents/100).arg(cents%100,2,10,QChar('0'));
}
//just to look pretty, I won't bother implementing languages, at least I don't think I will.
enum class Language{
    English
};

struct TicketData{
    QString name;
    Cents price;
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

class MainPage:public QWidget{
    Q_OBJECT
    QVBoxLayout layout;

    //goes without saying this is only for this version of the app
    QPushButton* debugEditCoinsBtn =new QPushButton("DEBUG edit coins contents",this);
    //languages are a non-functional placeholde (since a page with a single option seems redundant,and I still want to have a main page)
    QPushButton* languagesBtn =new QPushButton("Languages",this);
    QPushButton* purchaseBtn = new QPushButton("Buy ticket",this);
    QPushButton* exitBtn=new QPushButton("EXIT",this);

public:
    MainPage():layout(this){
        layout.addWidget(debugEditCoinsBtn);
        layout.addWidget(languagesBtn);
        layout.addWidget(purchaseBtn);
        layout.addWidget(exitBtn);
        connect(debugEditCoinsBtn,&QPushButton::clicked,this,&MainPage::debugEditCoinsOption);
        connect(languagesBtn,&QPushButton::clicked,this,&MainPage::languagesOption);
        connect(purchaseBtn,&QPushButton::clicked,this,&MainPage::purchaseOption);
        //it's fine here to hard quit because in real hardware this button would not exist
        connect(exitBtn,&QPushButton::clicked,&QApplication::quit);
    }
signals:
    void debugEditCoinsOption();
    void languagesOption();
    void purchaseOption();
};

class DebugEditCoins:public QWidget{
    Q_OBJECT
    QVBoxLayout layout;

    QPushButton* backButton=new QPushButton("Back",this);
    QHash<int,QLineEdit*> denomination_AmountDisplay;
    QHash<int,QPushButton*> denomination_SubtractButton;
public:
    DebugEditCoins():layout(this){
        for(Cents denominationVal:CoinInventory::acceptedDenominationsCents){
            QHBoxLayout* rowLayout=new QHBoxLayout;
            QString denominationStr=QString::number(denominationVal);
            QLabel* denominationLabel=new QLabel(denominationStr);
            QLineEdit* currentValue=new QLineEdit("0");
            currentValue->setReadOnly(true);
            currentValue->setFocusPolicy(Qt::NoFocus);
            currentValue->setFixedWidth(50);
            QPushButton* subBtn=new QPushButton("-");
            subBtn->setFixedWidth(50);
            QPushButton* addBtn=new QPushButton("+");
            addBtn->setFixedWidth(50);

            connect(subBtn,&QPushButton::clicked,this,[this,denominationVal](){emit DEBUGcoinRemoved(denominationVal);});
            connect(addBtn,&QPushButton::clicked,this,[this,denominationVal](){emit DEBUGcoinAdded(denominationVal);});

            denomination_AmountDisplay[denominationVal]=currentValue;
            denomination_SubtractButton[denominationVal]=subBtn;
            subBtn->setDisabled(true);

            rowLayout->addWidget(denominationLabel);
            rowLayout->addStretch();
            rowLayout->addWidget(currentValue);
            rowLayout->addWidget(subBtn);
            rowLayout->addWidget(addBtn);

            layout.addLayout(rowLayout);
        }
        connect(backButton,&QPushButton::clicked,this,&DebugEditCoins::backPressed);
        layout.addWidget(backButton);
    }
public slots:
    void setAmountValues(CoinInventory inventory){
        for(const auto& [denomination,amount]:inventory.getInventoryList()){
            denomination_AmountDisplay[denomination]->setText(QString::number(amount));
            QPushButton* subBtn=denomination_SubtractButton[denomination];
            if(amount==0)subBtn->setDisabled(true);
            else subBtn->setDisabled(false);
        }
    }
signals:
    void backPressed();
    void DEBUGcoinAdded(Cents denomination);
    void DEBUGcoinRemoved(Cents denomination);
    
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

class ReturningMoneyPage:public QWidget{
    Q_OBJECT
    QVBoxLayout layout;

    QLabel* couldNotProduceChangeMessage=new QLabel("Sorry, the machine could not produce exact change");
    QLabel* pleaseWaitMessage=new QLabel("Returning inserted coins. Please wait.");
public:

    ReturningMoneyPage():layout(this){
        layout.addWidget(couldNotProduceChangeMessage);
        layout.addWidget(pleaseWaitMessage);
    }
    void reinitialize(bool couldNotGiveChange){
        if(couldNotGiveChange)couldNotProduceChangeMessage->setVisible(true);
        else couldNotProduceChangeMessage->setVisible(false);
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

    //This is obviously an abstraction of a physical process. Real implementation will need data to know what we're printing. Printing introduces edgecases, but we don't worry about that now.
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
CoinInventory localInventory{};

CoinInventory transactionInventory{};

Cents ticketCost=0;

private:

void outputCoins(CoinInventory coins){
    for(const auto& [denomination,amount]:coins.getInventoryList()){
        qInfo()<<amount<<" coins of denomination "<<denomination<<" returned";    
    }
}

public:
PaymentProcessor(QObject* parent=nullptr):QObject(parent){}

//as a placeholder for now we assume we can't give out change, unless there is no need to return any. 
std::optional<CoinInventory> canGiveOutChange(){
    if(transactionInventory.getTotalAmount()==ticketCost)return CoinInventory{};

    CoinInventory fullInventory=localInventory+transactionInventory;
    //todo
    return {};
}

void loadLocalInventory(CoinInventory inventory){
    if(inventory!=localInventory){
        localInventory = inventory;
        emit localInventoryChanged(localInventory);
    }
}

signals:

void localInventoryChanged(CoinInventory);
void amountInsertedChanged(Cents newValue,bool isEnough);
void invalidCoinInserted();
void purchaseCompleted();
void returningCoins(bool couldNotGiveOutExactChange);
void coinsReturned();

public slots:
    void DEBUGCoinAdded(Cents cents){
        localInventory.addCoin(cents);
        emit localInventoryChanged(localInventory);
    }

    void DEBUGCoinRemoved(Cents cents){
        localInventory.subtractCoin(cents);
        emit localInventoryChanged(localInventory);
    }

    void insertCoin(Cents coinVal){
        transactionInventory.addCoin(coinVal);
        emit amountInsertedChanged(transactionInventory.getTotalAmount(),transactionInventory.getTotalAmount()>=ticketCost);
    }

    //placeholder. For now accept blindly
    void tryAcceptPurchase(){
        Cents sumInserted=transactionInventory.getTotalAmount();
        if(sumInserted<ticketCost){
            //todo emit ... (this is an error state) temporary fix:
            cancelTransaction(true);
        }else{
            auto change=canGiveOutChange();
            if(!change){
                cancelTransaction(true);
            }else{
                localInventory.moveInventoryFrom(transactionInventory); 
                outputCoins(change.value());
                localInventory.subtractInventory(change.value());

                if(sumInserted!=0)emit localInventoryChanged(localInventory);
                emit purchaseCompleted();
            }
        }
    }

    void cancelTransaction(bool couldNotGiveOutExactChange = false){
        emit returningCoins(couldNotGiveOutExactChange);
        outputCoins(transactionInventory);
        transactionInventory={};
        //abstraction, it would take time for real machine to spit out all the inserted coins back
        QTimer::singleShot(2000, this, [this] {
            emit coinsReturned();
        });
    }

    void startTransaction(Cents cost){
        if(transactionInventory.getTotalAmount()!=0)qFatal("Multiple transactions at the same time");//A little hard-handed. Just a temporary solution
        else ticketCost=cost;
    }
};

class SessionController:public QObject{
Q_OBJECT
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

    //Ultimatly this will be a server call, so for now this is a bare-bones placeholder.
    bool verifyName(QString name){
        if(name.isEmpty())return false;
        return true;
    }

    //placeholder
    CoinInventory getLocalCoinInventory(QString dbId){
        CoinInventory result;
        result.addCoin(5);    //$0.05 
        result.addCoin(100,2);  //$1 * 2
        return result;
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
        connect(paymentProcessor,&PaymentProcessor::localInventoryChanged,this,&SessionController::localInventoryChanged);
        connect(paymentProcessor,&PaymentProcessor::returningCoins,this,&SessionController::returningCoins);
        connect(paymentProcessor,&PaymentProcessor::coinsReturned,this,&SessionController::coinsReturned);
    }

public slots:
    
    void DEBUGCoinAdded(Cents coin){
        paymentProcessor->DEBUGCoinAdded(coin);
    }

    void DEBUGCoinRemoved(Cents coin){
        paymentProcessor->DEBUGCoinRemoved(coin);
    }

    //for now placeholder, later try to log into the local db and retrieve real data. /
    //The DB is just local storage, but since this is just a simulation of a real machine, /
    //there can be multiple machines with multiple DBs simulated on a single PC so we have to differentiate them
    void dbIdProvided(QString dbId){
        if(dbId=="temp"){
            paymentProcessor->loadLocalInventory(getLocalCoinInventory(dbId));
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
    }

    void tryCoin(QString coin){
        if(CoinInventory::isValidDenomination(coin)==false)emit invalidCoinInserted();
        else{
            paymentProcessor->insertCoin(coin.toUInt());
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
    void localInventoryChanged(CoinInventory);
    void unknownDBId();
    void nameAccepted(TicketData);
    void nameRejected();
    void amountInsertedChanged(Cents,bool isEnough);
    void sessionReady();
    void invalidCoinInserted();
    void purchaseCompleted();
    void ticketChoiceAccepted(TicketData);
    void returningCoins(bool couldNotGiveOutExactChange);
    void coinsReturned();

};

class MainWindow:public QWidget{
    Q_OBJECT

    QVBoxLayout layout;
    QStackedWidget stack;

    LoginPage* loginPage;
    MainPage* mainPage;
    DebugEditCoins* debugEditCoins;
    LanguagesPage* languagesPage;
    ChooseTicketPage* chooseTicketPage;
    InputPersonalDataPage* inputPersonalDataPage;
    PaymentPage* paymentPage;
    ReturningMoneyPage* returningMoneyPage;
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

        
        debugEditCoins=new DebugEditCoins;
        stack.addWidget(debugEditCoins);
        connect(mainPage,&MainPage::debugEditCoinsOption,this,&MainWindow::goToDebugEditCoins);
        connect(debugEditCoins,&DebugEditCoins::backPressed,this,&MainWindow::goToMain);
        connect(debugEditCoins,&DebugEditCoins::DEBUGcoinAdded,session,&SessionController::DEBUGCoinAdded);
        connect(debugEditCoins,&DebugEditCoins::DEBUGcoinRemoved,session,&SessionController::DEBUGCoinRemoved);
        connect(session,&SessionController::localInventoryChanged,debugEditCoins,&DebugEditCoins::setAmountValues);


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


        returningMoneyPage=new ReturningMoneyPage();
        stack.addWidget(returningMoneyPage);
        connect(session,&SessionController::returningCoins,this,&MainWindow::goToReturningMoneyPage);
        connect(session,&SessionController::coinsReturned,this,&MainWindow::goToMain);

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

    void goToReturningMoneyPage(bool couldNotGiveOutChange=false){
        returningMoneyPage->reinitialize(couldNotGiveOutChange);
        stack.setCurrentWidget(returningMoneyPage);
    }

    void goToDebugEditCoins(){
        stack.setCurrentWidget(debugEditCoins);
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
