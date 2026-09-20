#include <QCoreApplication>
#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QTimer>
#include <QObject>
#include <QTcpSocket>

#include "protocol/serialization.h"
#include "protocol/constants.h"


struct CoinInventory{
    static constexpr std::array<Cents,8> acceptedDenominationsCents{1,5,10,25,100,500,1000,2000};
    static_assert(std::ranges::is_sorted(acceptedDenominationsCents));
    static_assert(std::ranges::adjacent_find(acceptedDenominationsCents) == acceptedDenominationsCents.end());
    static_assert(acceptedDenominationsCents[0]>0);
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
        Q_ASSERT(isValidDenomination(value));
        amountOf(value)+=amount;
    }
    void subtractCoin(Cents value,quint64 amount=1){
        Q_ASSERT(isValidDenomination(value));
        Q_ASSERT(amountOf(value)>=amount);
        amountOf(value)-=amount;
    }

    std::vector<std::pair<Cents,quint64>> getInventoryList()const{
        std::vector<std::pair<Cents,quint64>> result;
        result.reserve(inventory.size());
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
    
    quint64 getAmountOfCoins() const{
        quint64 result=0;
        for(quint64 amount:inventory)result+=amount;
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

    std::optional<CoinInventory> coinChange(Cents change) const{
        static constexpr Cents maximumChange=50000; //500$ protection agains abuse, since dp can theoreticaly consume a lot of time and memory. If someone inserts that much money, he's not serious. We can safely reject it.
        static constexpr Cents maximumCoinsDispensed=100; //protection against expensive calculations, and we don't want to flood user with too many coins anyway.
        if(change==0)return CoinInventory{};
        if(change>maximumChange)return {}; 
        CoinInventory boundedInventory=*this;
        for(auto& count:boundedInventory.inventory)count=std::min(count,maximumCoinsDispensed); 

        std::vector<std::optional<CoinInventory>> oldDp(change+1);
        std::vector<std::optional<CoinInventory>> dp(change+1);
        dp[0]=CoinInventory{};
        for(int coinIndex=acceptedDenominationsCents.size()-1;coinIndex>=0;coinIndex--){
            oldDp=dp;
            for(quint64 coinAmount=1;coinAmount<=boundedInventory.inventory[coinIndex];coinAmount++){
                quint64 groupValue=coinAmount*acceptedDenominationsCents[coinIndex];
                for(size_t i=groupValue;i<dp.size();i++){
                    auto& previous = oldDp[i - groupValue];
                    if(!previous)continue;
                    const quint64 potentialCoinCount=previous->getAmountOfCoins() + coinAmount;
                    if(potentialCoinCount>maximumCoinsDispensed)continue;
                    if(!dp[i] or (dp[i]->getAmountOfCoins()>potentialCoinCount)){
                        dp[i]=previous;
                        dp[i]->inventory[coinIndex]+=coinAmount;
                    }
                }
            }
        }
        return dp[change];
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

enum TransactionCancelationReason{
    CouldNotGiveOutChange,
    UserRequested,
    Error
};

struct TicketData{
    TicketId Id; //alias of an integral type defined in constants.h for compatibility with server
    QString name;
    Cents price; //same as above
    bool isAvailable=true;
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
        loginButton->setDisabled(true);
        tryAgainMessage->setVisible(false);
        connect(loginButton,&QPushButton::clicked,this,&LoginPage::loginClicked);
    }
public slots:
    void dbIdRejected(){
        tryAgainMessage->setVisible(true);
        inputBox->setDisabled(false);
        loginButton->setDisabled(false);
    }
    void allowLogin(){
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
    QHash<Cents,QLineEdit*> denomination_AmountDisplay;
    QHash<Cents,QPushButton*> denomination_SubtractButton;
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
                QString buttonText=ticket.name;
                if(ticket.isAvailable==false)buttonText+=" - SOLD OUT";
                else buttonText+=" - "+centsToPriceString(ticket.price);
                QPushButton* btn=new QPushButton(buttonText,this);
                btn->setEnabled(ticket.isAvailable);
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
    QLabel* errorMessage=new QLabel("The name you provided has been rejected. Try again.",this);
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

    QLabel* reasonMessage=new QLabel;
    QLabel* pleaseWaitMessage=new QLabel("Returning inserted coins. Please wait.");
public:

    ReturningMoneyPage():layout(this){
        layout.addWidget(reasonMessage);
        layout.addWidget(pleaseWaitMessage);
    }
    void reinitialize(TransactionCancelationReason reason){
        reasonMessage->setVisible(true);
        switch (reason){
        case TransactionCancelationReason::CouldNotGiveOutChange:
            reasonMessage->setText("Sorry, the machine could not produce the exact change");
            break;
        case TransactionCancelationReason::UserRequested:
            reasonMessage->setVisible(false);
            break;
        case TransactionCancelationReason::Error:
            //continue to the default case
        default:
            reasonMessage->setText("Sorry, something went wrong. Returning inserted money");
            break;
        }
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

enum class PaymentProcessorState{
    idle,
    acceptingCoins,
    awaitingFinalConfirmation
};

class PaymentProcessor:public QObject{
    Q_OBJECT
    CoinInventory localInventory{};
    CoinInventory transactionInventory{};
    CoinInventory changeToGiveOut{};
    Cents ticketCost=0;
    PaymentProcessorState currentState=PaymentProcessorState::idle;
private:

    void outputCoins(CoinInventory coins){
    for(const auto& [denomination,amount]:coins.getInventoryList()){
        qInfo()<<amount<<" coins of denomination "<<denomination<<" returned";    
    }
}

public:
    PaymentProcessor(QObject* parent=nullptr):QObject(parent){}

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
    void returningCoins(TransactionCancelationReason);
    void coinsReturned();
    void canNotGiveOutChange();
    void changeReady();
    void errorWhenPreparingChange();
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
        if(currentState==PaymentProcessorState::acceptingCoins){
            transactionInventory.addCoin(coinVal);
            emit amountInsertedChanged(transactionInventory.getTotalAmount(),transactionInventory.getTotalAmount()>=ticketCost);
        }else qFatal("coin inserted during wrong state");//todo
    }

    void prepareToAcceptPurchase(){
        if(currentState!=PaymentProcessorState::acceptingCoins){
            emit errorWhenPreparingChange();
        }else{
            Cents sumInserted=transactionInventory.getTotalAmount();
            if(sumInserted<ticketCost){
                qWarning()<<"not enough money to afford the ticket";
                emit canNotGiveOutChange();
            }else{
                Cents changeValue=sumInserted-ticketCost;
                CoinInventory fullInventory=localInventory+transactionInventory;
                auto change=fullInventory.coinChange(changeValue);
                if(!change){
                    emit canNotGiveOutChange();
                }else{
                    changeToGiveOut=change.value();
                    currentState=PaymentProcessorState::awaitingFinalConfirmation;
                    emit changeReady();
                }
            }
        }
    }

    void finalizePurchase(){
        if(currentState!=PaymentProcessorState::awaitingFinalConfirmation){
            qFatal("finalizePurchase called when not awaitingFinalConfirmation");//todo
        }else{
            localInventory.moveInventoryFrom(transactionInventory); 
            outputCoins(changeToGiveOut);
            localInventory.subtractInventory(changeToGiveOut);
            changeToGiveOut={};
            currentState=PaymentProcessorState::idle;
            emit localInventoryChanged(localInventory);
            emit purchaseCompleted();
        }
    }

    void cancelTransaction(TransactionCancelationReason reason){
        emit returningCoins(reason);
        outputCoins(transactionInventory);
        transactionInventory={};
        currentState=PaymentProcessorState::idle;
        //abstraction, it would take time for real machine to spit out all the inserted coins back
        QTimer::singleShot(2000, this, [this] {
            emit coinsReturned();
        });
    }

    void startTransaction(Cents cost){
        if(transactionInventory.getTotalAmount()!=0)qFatal("Multiple transactions at the same time");//A little hard-handed. Just a temporary solution
        else {
            currentState=PaymentProcessorState::acceptingCoins;
            ticketCost=cost;
        }
    }
};

class ServerConnection:public QObject{
    static constexpr int MAX_MESSAGE_SIZE = 4096;
    static constexpr int MAX_BUFFER_SIZE = MAX_MESSAGE_SIZE*10;
Q_OBJECT
    QTcpSocket socket;
    QByteArray buffer;

    //When I'm finished with the current part I'll revisit and look at all calls individually. Should be broken down into multiple functions.
    //Also criticly, remamber to eject all current transaction money before shutdown
    [[deprecated("placeholder function")]] void unrecorevableError_todo(QByteArray hint){
        qFatal("paceholder, unrecorevable error: %s", qPrintable(hint));
    }

    QByteArray frameRequest(const ClientMessage& message){
        QByteArray result;
        quint32 size=message.message.size()+sizeof(message.type);

        result.reserve(size+sizeof(size));
        
        result.append(parsing::packNumber(size));
        result.append(parsing::packNumber((quint16)message.type));
        result.append(message.message);

        return result;
    }

    //note to consider error-handeling, but it's fine to leave it for later
    void handleRawTicketListData(const QByteArray& data){
        std::vector<TicketData> result;
        quint32 offset=0;
        quint8 numberOfTickets = parsing::unpackNumber<quint8>(data,offset);
        for(int i=0;i<numberOfTickets;i++){
            TicketId Id = parsing::unpackNumber<TicketId>(data,offset);
            QByteArray name=parsing::unpack8BitPrefixedByteArray(data,offset);
            Cents cost = parsing::unpackNumber<Cents>(data,offset);
            bool isAvailable=parsing::unpackNumber<quint8>(data,offset);
            result.push_back(TicketData{Id,name,cost,isAvailable});
        }
        emit ticketListUpdated(result);
    };

    void handleServerMessage(const QByteArray& data){
        ServerResponse framedResponse=parsing::unpackServerResponse(data);

        //the following two checks are placeholders. Their bodies are to be eventually replaced.
        if(serverMessageCheckCategory(framedResponse.type,ServerMessageCategory::ServerCrit)){
            unrecorevableError_todo("Something went __seriously__ wrong with the server and it had to shut down");
        }else if(serverMessageCheckCategory(framedResponse.type,ServerMessageCategory::ClientCrit)){
            unrecorevableError_todo("Something went __seriously__ wrong and server requested a shut down. Contact the administrator");
        }
        
        switch (framedResponse.inResponseTo){
            case ClientMessageType::REQUEST_Version_validation:
                if(framedResponse.type==ServerMessageType::OK)emit versionValidated();
                else unrecorevableError_todo("Version_validation not OK");
                break;
                case ClientMessageType::REQUEST_Get_ticket_list:
                if(framedResponse.type==ServerMessageType::OK)handleRawTicketListData(framedResponse.message);
                else unrecorevableError_todo("Get_ticket_list not OK");
                break;
            case ClientMessageType::REQUEST_Start_checkout:
                if(framedResponse.type==ServerMessageType::OK){
                    emit checkOutStarted(parsing::unpackNumber<TicketId>(framedResponse.message));
                }else unrecorevableError_todo("Start_checkout not OK");
                break;
            case ClientMessageType::REQUEST_Buy:
                if(framedResponse.type==ServerMessageType::OK){
                    emit purchaseRecorded(parsing::unpackNumber<TicketId>(framedResponse.message));
                }else unrecorevableError_todo("Buy not OK");
                break;
            case ClientMessageType::REQUEST_Cancel_checkout:
                if(framedResponse.type==ServerMessageType::OK){
                    emit checkoutCanceled();
                }else unrecorevableError_todo("Cancel_checkout not OK");
                break;
            case ClientMessageType::REQUEST_Validate_name:
                if(framedResponse.type==ServerMessageType::OK){
                    bool response = parsing::unpackNumber<quint8>(framedResponse.message);
                    if(response==true)emit nameAccepted();
                    else emit nameRejected();
                }else unrecorevableError_todo("Validate_name not OK");
                break;
            default:
                unrecorevableError_todo("response to unknown request type");
        }
    }

    void onReadyRead(){
        buffer+=socket.readAll();
        if(buffer.size()>MAX_BUFFER_SIZE){
            unrecorevableError_todo("server flooded machine");
        }
        while(true){
            if(buffer.size()<4)break;
            
            quint32 len=parsing::unpackNumber<quint32>(buffer);
            if(len>MAX_MESSAGE_SIZE){
                unrecorevableError_todo("too long message from server");
                return;
            } 
            quint32 totalLen=len+sizeof(quint32);
            if(buffer.size()<totalLen)break;
            QByteArray rawMessage = buffer.mid(4,len);
            buffer.remove(0,totalLen);
            
            if(rawMessage.size()<4){
                unrecorevableError_todo("server message too short");
            }else{
                handleServerMessage(rawMessage);
            }
        }

    }

public:

    ServerConnection(QObject* parent=nullptr):QObject(parent),socket(this){
        connect(&socket, &QTcpSocket::readyRead,this,&ServerConnection::onReadyRead);
    }

public slots:
    void validateVersion(){
        socket.write(frameRequest({ClientMessageType::REQUEST_Version_validation,protocolVersion}));
    }
    void connectToServer(){
        socket.connectToHost("127.0.0.1", 12345);
        if(socket.waitForConnected(5000)){// wait 5s in case of a slow connection
            emit serverReady();
        }else{
            emit failedToConnectToServer();
        }
    }
    void requestTicketList(){
        socket.write(frameRequest({ClientMessageType::REQUEST_Get_ticket_list}));
    }
    void startTransaction(TicketId id){
        socket.write(frameRequest({ClientMessageType::REQUEST_Start_checkout,parsing::packNumber<TicketId>(id)}));
    }
    void cancelTransaction(){
        socket.write(frameRequest({ClientMessageType::REQUEST_Cancel_checkout}));
    };
    void validateName(QByteArray name){
        socket.write(frameRequest({ClientMessageType::REQUEST_Validate_name,parsing::pack8BitPrefixedByteArray(name)}));
    }
    void finalizePurchase(QByteArray name,TicketId id){
        QByteArray message="";
        message.append(parsing::pack8BitPrefixedByteArray(name));
        message.append(parsing::packNumber<TicketId>(id));
        socket.write(frameRequest({ClientMessageType::REQUEST_Buy,message}));
    }
signals:
    void checkoutCanceled();
    void purchaseRecorded(TicketId);
    void checkOutStarted(TicketId);
    void failedToConnectToServer();
    void versionValidated();
    void serverReady();
    void ticketListUpdated(std::vector<TicketData>);
    void nameAccepted();
    void nameRejected();
};

class SessionController:public QObject{
Q_OBJECT
private:
    Language language=Language::English;    
    std::vector<TicketData> availableTickets;
    TicketData currentTicket;
    PaymentProcessor* paymentProcessor;
    ServerConnection* serverConnection; 
    QByteArray buyerName;
    bool serverInitiated=false;

    //Ultimatly this will be a server call, so for now this is a bare-bones placeholder.
    bool verifyName(QString name){
        if(name.isEmpty())return false;
        buyerName = name.toUtf8();
        return true;
    }

    //placeholder
    CoinInventory getLocalCoinInventory(QString dbId){
        CoinInventory result;
        result.addCoin(1,10);    //$0.01 *10
        result.addCoin(5,2);
        result.addCoin(25,7);
        result.addCoin(100);  //$1 * 1
        return result;
    }


public:
    const TicketData& getCurrentTicket() const{
        return currentTicket;
    }

    const std::vector<TicketData>& getAvailableTickets() const {
        return availableTickets;
    }

    Language getLanguage() const {
        return language;
    }
    
    SessionController(QObject* parent=nullptr):QObject(parent){
        paymentProcessor= new PaymentProcessor(this);
        serverConnection= new ServerConnection(this);
        connect(paymentProcessor,&PaymentProcessor::amountInsertedChanged,this,&SessionController::amountInsertedChanged);
        connect(paymentProcessor,&PaymentProcessor::localInventoryChanged,this,&SessionController::localInventoryChanged);
        connect(paymentProcessor,&PaymentProcessor::returningCoins,       this,&SessionController::returningCoins);
        connect(paymentProcessor,&PaymentProcessor::coinsReturned,        this,&SessionController::coinsReturned);

        connect(paymentProcessor,&PaymentProcessor::canNotGiveOutChange,     this,&SessionController::cannotGiveOutChange);
        connect(paymentProcessor,&PaymentProcessor::errorWhenPreparingChange,this,&SessionController::errorWhenPreparingChange);
        connect(paymentProcessor,&PaymentProcessor::changeReady,             this,&SessionController::changeReady);
        
        connect(serverConnection,&ServerConnection::checkOutStarted, this,&SessionController::serverAcceptedTicket);
        connect(serverConnection,&ServerConnection::purchaseRecorded,paymentProcessor,&PaymentProcessor::finalizePurchase);
        connect(paymentProcessor,&PaymentProcessor::purchaseCompleted,this,&SessionController::purchaseCompleted);
        connect(serverConnection,&ServerConnection::serverReady,serverConnection,&ServerConnection::validateVersion);
        connect(serverConnection,&ServerConnection::nameAccepted,this,&SessionController::nameAccepted);
        connect(serverConnection,&ServerConnection::nameRejected,this,&SessionController::nameRejected);

        //after version has been validated server should send updates to the ticketList on it's own (that part is not hooked up yet), but we need the initial list.
        connect(serverConnection,&ServerConnection::versionValidated,serverConnection,&ServerConnection::requestTicketList);
        
        connect(serverConnection,&ServerConnection::ticketListUpdated,this,&SessionController::updateTicketList);
        
        //should probably put UI in "sleep mode" and retry from time to time, but I can write that last. For now this is a fine placeholder:
        connect(serverConnection,&ServerConnection::failedToConnectToServer,this,[](){
            qFatal("paceholder, You probably forgot to fire up the server first");
        });
    }



public slots:
    void updateTicketList(std::vector<TicketData> list){
        availableTickets=list;
        if(serverInitiated)emit ticketListChanged(availableTickets);
        else{
            serverInitiated=true;
            emit serverReady();
        }
    }
    
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

    void tryCoin(QString coin){
        if(CoinInventory::isValidDenomination(coin)==false)emit invalidCoinInserted();
        else{
            paymentProcessor->insertCoin(coin.toULongLong());
        }
    }

    void tryAcceptPurchase(){
        paymentProcessor->prepareToAcceptPurchase();
    }

    void startTransaction(){
        paymentProcessor->startTransaction(currentTicket.price);
    }

    void ticketPicked(TicketData data){
        currentTicket=data;
        serverConnection->startTransaction(data.Id);
    }

    void validateBuyerName(QString name){
        buyerName=name.toUtf8();
        serverConnection->validateName(name.toUtf8());
    }

    void onUIReady(){
        serverConnection->connectToServer();
    }

    void serverAcceptedTicket(TicketId id){
        if(currentTicket.Id==id)emit ticketChoiceAccepted(currentTicket);
        else{
            //todo
        }
    }

    void userCanceledTransaction(){
        serverConnection->cancelTransaction();
        paymentProcessor->cancelTransaction(TransactionCancelationReason::UserRequested);
    }

    void cannotGiveOutChange(){
        serverConnection->cancelTransaction();
        paymentProcessor->cancelTransaction(TransactionCancelationReason::CouldNotGiveOutChange);
    }

    void errorWhenPreparingChange(){
        serverConnection->cancelTransaction();
        paymentProcessor->cancelTransaction(TransactionCancelationReason::Error);
    }

    void changeReady(){
        serverConnection->finalizePurchase(buyerName,currentTicket.Id);
    }

signals:
    void restartStatus();
    void ticketListChanged(std::vector<TicketData>);
    void localInventoryChanged(CoinInventory);
    void unknownDBId();
    void nameAccepted();
    void nameRejected();
    void amountInsertedChanged(Cents,bool isEnough);
    void sessionReady();
    void invalidCoinInserted();
    void purchaseCompleted();
    void ticketChoiceAccepted(TicketData);
    void returningCoins(TransactionCancelationReason);
    void coinsReturned();

    void serverReady();
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
        connect(session,&SessionController::restartStatus,this,&MainWindow::goToMain);

        //stack.addWidget() passes ownership to the stack immidietly after the 'new'
        loginPage=new LoginPage;
        stack.addWidget(loginPage);
        connect(loginPage,&LoginPage::dataBaseIDProvided,session,&SessionController::dbIdProvided);
        connect(session,&SessionController::unknownDBId,loginPage,&LoginPage::dbIdRejected);
        connect(session,&SessionController::serverReady,loginPage,&LoginPage::allowLogin);


        mainPage=new MainPage;
        stack.addWidget(mainPage);
        connect(mainPage,&MainPage::languagesOption,this,&MainWindow::goToLanguages);
        connect(mainPage,&MainPage::purchaseOption, this,&MainWindow::goToChooseTicketPage);

        
        debugEditCoins=new DebugEditCoins;
        stack.addWidget(debugEditCoins);
        connect(mainPage,&MainPage::debugEditCoinsOption,   this,&MainWindow::goToDebugEditCoins);
        connect(debugEditCoins,&DebugEditCoins::backPressed,this,&MainWindow::goToMain);
        connect(debugEditCoins,&DebugEditCoins::DEBUGcoinAdded,  session,&SessionController::DEBUGCoinAdded);
        connect(debugEditCoins,&DebugEditCoins::DEBUGcoinRemoved,session,&SessionController::DEBUGCoinRemoved);
        connect(session,&SessionController::localInventoryChanged,debugEditCoins,&DebugEditCoins::setAmountValues);


        languagesPage=new LanguagesPage;
        stack.addWidget(languagesPage);
        connect(languagesPage,&LanguagesPage::languagePicked,session,&SessionController::languageChanged);
        connect(languagesPage,&LanguagesPage::backPressed,   this,&MainWindow::goToMain);


        chooseTicketPage=new ChooseTicketPage();
        stack.addWidget(chooseTicketPage);
        connect(chooseTicketPage,&ChooseTicketPage::backPressed ,this,&MainWindow::goToMain);
        connect(chooseTicketPage,&ChooseTicketPage::ticketPicked,session,&SessionController::ticketPicked);
        connect(session,&SessionController::ticketChoiceAccepted,this,&MainWindow::goToInputPersonalDataPage);


        inputPersonalDataPage=new InputPersonalDataPage();
        stack.addWidget(inputPersonalDataPage);
        connect(inputPersonalDataPage,&InputPersonalDataPage::cancelPressed        ,session,&SessionController::userCanceledTransaction);
        connect(inputPersonalDataPage,&InputPersonalDataPage::personalDataSubmitted,session,&SessionController::validateBuyerName);
        connect(session,&SessionController::nameRejected,inputPersonalDataPage,&InputPersonalDataPage::submittedNameNotAccepted);
        connect(session,&SessionController::nameAccepted,this,&MainWindow::goToTakeCoinsPage);


        paymentPage=new PaymentPage();
        stack.addWidget(paymentPage);
        connect(paymentPage,&PaymentPage::cancelPressed,       session,&SessionController::userCanceledTransaction);
        connect(paymentPage,&PaymentPage::denominationInserted,session,&SessionController::tryCoin);
        connect(paymentPage,&PaymentPage::confirmPressed,      session,&SessionController::tryAcceptPurchase);
        connect(session,&SessionController::invalidCoinInserted,  paymentPage,&PaymentPage::unknownCoin);
        connect(session,&SessionController::amountInsertedChanged,paymentPage,&PaymentPage::amountInsertedChanged);
        connect(session,&SessionController::purchaseCompleted,this,&MainWindow::goToPrintingPage);


        returningMoneyPage=new ReturningMoneyPage();
        stack.addWidget(returningMoneyPage);
        connect(session,&SessionController::returningCoins,this,&MainWindow::goToReturningMoneyPage);
        connect(session,&SessionController::coinsReturned, this,&MainWindow::goToMain);

        printingPage=new PrintingPage;
        stack.addWidget(printingPage);
        connect(printingPage,&PrintingPage::printingFinished,session,&SessionController::logPrinted);


        stack.setCurrentWidget(loginPage);
        layout.addWidget(&stack);
        session->onUIReady();
    }
public slots:

    //conceptually we'll call reinitialize on all of them, while also passing the current language, but now it's not necessary.
    void goToMain(){
        stack.setCurrentWidget(mainPage);
    }

    void goToReturningMoneyPage(TransactionCancelationReason reason){
        returningMoneyPage->reinitialize(reason);
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

    void goToTakeCoinsPage(){
        paymentPage->reinitialize(session->getCurrentTicket());
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
signals:
};

int main(int argc, char *argv[]){
    QApplication app(argc, argv);
    MainWindow window;
    window.show();

    return app.exec();
}

#include "main.moc"
