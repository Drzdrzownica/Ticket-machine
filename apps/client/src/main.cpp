
#include <QCoreApplication>
#include <QApplication>
#include <QStackedWidget>
#include <QTimer>
#include <QTcpSocket>

#include "protocol/serialization.h"
#include "protocol/constants.h"
#include "logging.h"
#include "client/ClientTypes.h"
#include "client/UI/ErrorStatePage.h"
#include "client/UI/ErrorTryAgainPage.h"
#include "client/UI/LoginPage.h"
#include "client/UI/MainPage.h"
#include "client/UI/LanguagesPage.h"
#include "client/UI/ChooseTicketPage.h"
#include "client/UI/PaymentPage.h"
#include "client/UI/InputPersonalDataPage.h"
#include "client/UI/DebugEditCoinsPage.h"
#include "client/UI/ReturningMoneyPage.h"
#include "client/UI/PrintingPage.h"

//this is meant as client-side UI, meant to run on PC and simulate a version that will run on dedicated hardware.
enum class PaymentProcessorState{
    idle,
    acceptingCoins,
    awaitingFinalConfirmation,
    disabled //awaiting reboot
};

class PaymentProcessor:public QObject{
    Q_OBJECT
    CoinInventory localInventory{};
    CoinInventory transactionInventory{};
    CoinInventory changeToGiveOut{};
    Cents ticketCost=0;
    PaymentProcessorState currentState=PaymentProcessorState::idle;
private:

    void resetTransaction(){
        changeToGiveOut={};
        transactionInventory={};
        ticketCost = 0;
        currentState=PaymentProcessorState::idle;
    }

    void outputCoins(CoinInventory coins){
    for(const auto& [denomination,amount]:coins.getInventory()){
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
    void returningCoins(TransactionCancellationReason);
    void coinsReturned();
    void canNotGiveOutChange();
    void changeReady();
    void errorWhenPreparingChange();
    void requestedShutdown(ClientShutdownReason);
public slots:

    void panicEjectMoney(){
        outputCoins(transactionInventory);
        resetTransaction();
        currentState=PaymentProcessorState::disabled;
    }

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
        }else emit requestedShutdown(ClientShutdownReason::Transaction_state_violation);
    }

    void prepareToAcceptPurchase(){
        if(currentState!=PaymentProcessorState::acceptingCoins){
            emit errorWhenPreparingChange();
        }else{
            Cents sumInserted=transactionInventory.getTotalAmount();
            if(sumInserted<ticketCost){
                logging::log("Warning: not enough money to afford the ticket");
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
            emit requestedShutdown(ClientShutdownReason::Transaction_state_violation);
        }else{
            localInventory.moveInventoryFrom(transactionInventory); 
            outputCoins(changeToGiveOut);
            localInventory.subtractInventory(changeToGiveOut);
            resetTransaction();
            emit localInventoryChanged(localInventory);
            emit purchaseCompleted();
        }
    }

    void cancelTransaction(TransactionCancellationReason reason){
        emit returningCoins(reason);
        outputCoins(transactionInventory);
        resetTransaction();
        //abstraction, it would take time for real machine to spit out all the inserted coins back
        QTimer::singleShot(2000, this, [this] {
            emit coinsReturned();
        });
    }

    void startTransaction(Cents cost){
        if(currentState != PaymentProcessorState::idle)emit requestedShutdown(ClientShutdownReason::Transaction_state_violation);
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

    QByteArray frameRequest(const ClientMessage& message){
        QByteArray result;
        PacketLengthPrefix size=message.message.size()+sizeof(message.type);

        result.reserve(size+sizeof(size));
        
        result.append(parsing::packNumber(size));
        result.append(parsing::packNumber(static_cast<std::underlying_type_t<ClientMessageType>>(message.type)));
        result.append(message.message);

        return result;
    }

    //note to consider error-handeling, but it's fine to leave it for later
    void handleRawTicketListData(const QByteArray& data){
        try{
            std::vector<TicketData> result;
            qsizetype offset=0;
            quint8 numberOfTickets = parsing::unpackNumber<quint8>(data,offset);
            for(int i=0;i<numberOfTickets;i++){
                TicketId Id      = parsing::unpackNumber<TicketId>(data,offset);
                QByteArray name  = parsing::unpack8BitPrefixedByteArray(data,offset);
                Cents cost       = parsing::unpackNumber<Cents>(data,offset);
                bool isAvailable = parsing::unpackNumber<quint8>(data,offset);
                result.push_back(TicketData{Id,name,cost,isAvailable});
            }
            emit ticketListUpdated(result);
        }catch(...){
            emit requestedClientShutdown(ClientShutdownReason::Ticket_list_parsing_issue);
        }
    };

    void handleVersionValidationResponse(const ServerResponse& response){
        switch (response.type)
        {
        case ServerMessageType::OK:
            emit versionValidated();
            break;
        case ServerMessageType::CLIENT_WARNING_Version_already_validated:
            //no-op
            break;
        case ServerMessageType::CRIT_CLIENT_ERR_Version_mismatch:
            emit requestedClientShutdown(ClientShutdownReason::Version_validation_mismatch);
            break;
        default:
            emit requestedClientShutdown(ClientShutdownReason::Version_validation_issue);
            break;
        }
    }

    void handleGetTicketListResponse(const ServerResponse& response){
        switch (response.type)
        {
        case ServerMessageType::OK:
            handleRawTicketListData(response.message);
            break;
        default:
            emit requestedClientShutdown(ClientShutdownReason::Ticket_list_retrieval_issue);
            break;
        }
    }

    void handleValidateName(const ServerResponse& response){
        if(response.type==ServerMessageType::OK){
            bool accepted = parsing::unpackNumber<quint8>(response.message);
            if(accepted==true)emit nameAccepted();
            else emit nameRejected();
        //ValidateName theoretically has no way to fail and return error, for 'problematic' input (whatever that means) it should just return OK:false 
        }else emit requestedClientShutdown(ClientShutdownReason::Unexpected_response_type);
    }

    void handleStartCheckoutResponse(const ServerResponse& response){
        switch (response.type)
        {
        case ServerMessageType::OK:
        case ServerMessageType::CLIENT_WARNING_Ticket_already_in_checkout:
            emit checkOutStarted(parsing::unpackNumber<TicketId>(response.message));
            break;
        case ServerMessageType::ERR_No_tickets_in_stock_while_initiating_checkout:
        case ServerMessageType::ERR_Invalid_ticket_id:
            emit initiatingCheckoutRefused();
            break;
        default:
            emit requestedClientShutdown(ClientShutdownReason::Unexpected_response_type);
            break;
        }
    }

    void handleCancelCheckoutResponse(const ServerResponse& response){
        if(response.type==ServerMessageType::OK){
            emit checkoutCanceled();
        }else if(response.type==ServerMessageType::ERR_No_checkout_in_progress){
            //error is logged upon recival, we can act as if cancaling was a success.
            emit checkoutCanceled();
        }else{
            //there is no legitimate reason why server would refuse to cancel a partial transaction.
            emit requestedClientShutdown(ClientShutdownReason::Unexpected_response_type);
        }
    }

    //buy call always deletes transaction regardless of the success, no need for a separate call
    void handleBuyResponse(const ServerResponse& response){
        switch (response.type){
        case ServerMessageType::OK:
            emit purchaseRecorded(parsing::unpackNumber<TicketId>(response.message));
            break;
        case ServerMessageType::ERR_Item_not_in_checkout:
        //Wrong_item_in_checkout is iffy, but let's call this a recovery from an incorrect state
        case ServerMessageType::ERR_Wrong_item_in_checkout:
        case ServerMessageType::ERR_Ticket_no_longer_valid:
        //this one uses the same validation as the explicit validation call, so should never occur
        case ServerMessageType::ERR_Disallowed_name:
            emit purchaseFailed();
            break;
        default:
            emit requestedClientShutdown(ClientShutdownReason::Unexpected_response_type);
            break;
        }
    }

    void handleUnrecognizedMessageType(const ServerResponse& response){
        if(serverMessageCheckCategory(response.type,ServerMessageCategory::ClientWarning))return;//do nothing, it's logged anyway
        else if(serverMessageCheckCategory(response.type,ServerMessageCategory::Ok))return;//as above
        else emit requestedClientShutdown(ClientShutdownReason::Unexpected_response_type);
    }

    void handleServerMessage(const QByteArray& data){
        ServerResponse framedResponse;
        try{
            framedResponse=parsing::unpackServerResponse(data);
        }catch(...){
            emit requestedClientShutdown(ClientShutdownReason::Response_parsing_issue);
            return;
        }

        logging::log(framedResponse.logString());

        if( serverMessageCheckCategory(framedResponse.type,ServerMessageCategory::ServerCrit) || 
            serverMessageCheckCategory(framedResponse.type,ServerMessageCategory::ClientCrit)){
            //I might eventually replace with a function call that behaves slightly differently for each case (I haven't decided yet), but ultimately we will call the same function so it's not a priority.
            emit requestedClientShutdown(ClientShutdownReason::Generic_server_requested);
            return;
        }
        
        switch (framedResponse.inResponseTo){
            case ClientMessageType::REQUEST_Version_validation:
                handleVersionValidationResponse(framedResponse);
                break;
            case ClientMessageType::REQUEST_Get_ticket_list:
                handleGetTicketListResponse(framedResponse);
                break;
            case ClientMessageType::REQUEST_Start_checkout:
                handleStartCheckoutResponse(framedResponse);
                break;
            case ClientMessageType::REQUEST_Buy:
                handleBuyResponse(framedResponse);
                break;
            case ClientMessageType::REQUEST_Cancel_checkout:
                handleCancelCheckoutResponse(framedResponse);
                break;
            case ClientMessageType::REQUEST_Validate_name:
                handleValidateName(framedResponse);
                break;
            default:
                handleUnrecognizedMessageType(framedResponse);
        }
    }

    void onReadyRead(){
        buffer+=socket.readAll();
        if(buffer.size()>MAX_BUFFER_SIZE){
            emit requestedClientShutdown(ClientShutdownReason::Response_parsing_issue);
            return;
        }
        while(true){
            if(buffer.size()<sizeof(PacketLengthPrefix))break;
            
            quint64 len=parsing::unpackNumber<PacketLengthPrefix>(buffer);
            if(len>MAX_MESSAGE_SIZE){
                emit requestedClientShutdown(ClientShutdownReason::Response_parsing_issue);
                return;
            } 
            quint64 totalLen=len+sizeof(PacketLengthPrefix);
            if(buffer.size()<totalLen)break;
            QByteArray rawMessage = buffer.mid(sizeof(PacketLengthPrefix),len);
            buffer.remove(0,totalLen);
            
            handleServerMessage(rawMessage);
        }

    }
    void sendRequest(const ClientMessage &message){
        logging::log(message.logString());
        socket.write(frameRequest(message));
    }

public:

    ServerConnection(QObject* parent=nullptr):QObject(parent),socket(this){
        connect(&socket, &QTcpSocket::connected,this, &ServerConnection::onConnected);
        connect(&socket, &QTcpSocket::errorOccurred,this,  [this](){
            emit requestedClientShutdown(ClientShutdownReason::Socket_error);
        });
        connect(&socket, &QTcpSocket::disconnected,this, [this](){
            emit requestedClientShutdown(ClientShutdownReason::Server_disconnected);
        });
        connect(&socket, &QTcpSocket::readyRead,this,&ServerConnection::onReadyRead);
    }

public slots:
    void validateVersion(){
        sendRequest({ClientMessageType::REQUEST_Version_validation,protocolVersion});
    }
    void connectToServer(){
        socket.connectToHost("127.0.0.1", 12345);
    }
    void requestTicketList(){

        sendRequest({ClientMessageType::REQUEST_Get_ticket_list});
    }
    void startTransaction(TicketId id){
        sendRequest({ClientMessageType::REQUEST_Start_checkout,parsing::packNumber<TicketId>(id)});
    }
    void cancelTransaction(){
        sendRequest({ClientMessageType::REQUEST_Cancel_checkout});
    };
    void validateName(QByteArray name){
        sendRequest({ClientMessageType::REQUEST_Validate_name,parsing::pack8BitPrefixedByteArray(name)});
    }
    void finalizePurchase(QByteArray name,TicketId id){
        QByteArray message="";
        message.append(parsing::pack8BitPrefixedByteArray(name));
        message.append(parsing::packNumber<TicketId>(id));
        sendRequest({ClientMessageType::REQUEST_Buy,message});
    }
    void disconnectSocket(){
        socket.disconnectFromHost();
    }
signals:
    void requestedClientShutdown(ClientShutdownReason);
    void checkoutCanceled();
    void purchaseRecorded(TicketId);
    void checkOutStarted(TicketId);
    void versionValidated();
    void onConnected();
    void ticketListUpdated(std::vector<TicketData>);
    void nameAccepted();
    void nameRejected();
    void initiatingCheckoutRefused();
    void purchaseFailed();
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
    bool initialServerSyncComplete=false;

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
        connect(serverConnection,&ServerConnection::initiatingCheckoutRefused,this,&SessionController::ticketChoiceRejected);
        connect(serverConnection,&ServerConnection::purchaseRecorded,paymentProcessor,&PaymentProcessor::finalizePurchase);
        connect(serverConnection,&ServerConnection::purchaseFailed,this,[this](){
            paymentProcessor->cancelTransaction(TransactionCancellationReason::BuyFailed);
        });

        connect(paymentProcessor,&PaymentProcessor::purchaseCompleted,this,&SessionController::purchaseCompleted);
        connect(serverConnection,&ServerConnection::onConnected,serverConnection,&ServerConnection::validateVersion);
        connect(serverConnection,&ServerConnection::nameAccepted,this,&SessionController::nameAccepted);
        connect(serverConnection,&ServerConnection::nameRejected,this,&SessionController::nameRejected);
        connect(serverConnection,&ServerConnection::requestedClientShutdown,this,&SessionController::prepareShutdown);
        connect(paymentProcessor,&PaymentProcessor::requestedShutdown,this,&SessionController::prepareShutdown);


        //after version has been validated server should send updates to the ticketList on it's own (that part is not hooked up yet), but we need the initial list.
        connect(serverConnection,&ServerConnection::versionValidated,serverConnection,&ServerConnection::requestTicketList);
        
        connect(serverConnection,&ServerConnection::ticketListUpdated,this,&SessionController::updateTicketList);
        
        //should probably put UI in "sleep mode" and retry from time to time, but I can write that last. For now this is a fine placeholder:

    }

public slots:

    void requestTicketListUpdate(){
        serverConnection->requestTicketList();
    }

    void updateTicketList(std::vector<TicketData> list){
        availableTickets=list;
        if(initialServerSyncComplete)emit ticketListChanged(availableTickets);
        else{
            initialServerSyncComplete=true;
            emit serverReady();
        }
    }

    void prepareShutdown(ClientShutdownReason reason){
        paymentProcessor->panicEjectMoney();
        serverConnection->disconnectSocket();
        emit panicShutdown(reason);
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
        paymentProcessor->cancelTransaction(TransactionCancellationReason::UserRequested);
    }

    void cannotGiveOutChange(){
        serverConnection->cancelTransaction();
        paymentProcessor->cancelTransaction(TransactionCancellationReason::CouldNotGiveOutChange);
    }

    void errorWhenPreparingChange(){
        serverConnection->cancelTransaction();
        paymentProcessor->cancelTransaction(TransactionCancellationReason::Error);
    }

    void changeReady(){
        serverConnection->finalizePurchase(buyerName,currentTicket.Id);
    }

signals:
    void panicShutdown(ClientShutdownReason);
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
    void ticketChoiceRejected();
    void returningCoins(TransactionCancellationReason);
    void coinsReturned();

    void serverReady();
};

class MainWindow:public QWidget{
    Q_OBJECT

    QVBoxLayout layout;
    QStackedWidget stack;

    LoginPage* loginPage;
    ErrorStatePage* errorStatePage;
    MainPage* mainPage;
    DebugEditCoinsPage* debugEditCoins;
    LanguagesPage* languagesPage;
    ChooseTicketPage* chooseTicketPage;
    ErrorTryAgainPage* chooseTicketErrorPage;
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

        errorStatePage=new ErrorStatePage;
        stack.addWidget(errorStatePage);
        connect(session,&SessionController::panicShutdown,this,&MainWindow::goToErrorStatePage);


        mainPage=new MainPage;
        stack.addWidget(mainPage);
        connect(mainPage,&MainPage::languagesOption,this,&MainWindow::goToLanguages);
        connect(mainPage,&MainPage::purchaseOption, this,&MainWindow::goToChooseTicketPage);

        
        debugEditCoins=new DebugEditCoinsPage;
        stack.addWidget(debugEditCoins);
        connect(mainPage,&MainPage::debugEditCoinsOption,   this,&MainWindow::goToDebugEditCoins);
        connect(debugEditCoins,&DebugEditCoinsPage::backPressed,this,&MainWindow::goToMain);
        connect(debugEditCoins,&DebugEditCoinsPage::DEBUGcoinAdded,  session,&SessionController::DEBUGCoinAdded);
        connect(debugEditCoins,&DebugEditCoinsPage::DEBUGcoinRemoved,session,&SessionController::DEBUGCoinRemoved);
        connect(session,&SessionController::localInventoryChanged,debugEditCoins,&DebugEditCoinsPage::setAmountValues);


        languagesPage=new LanguagesPage;
        stack.addWidget(languagesPage);
        connect(languagesPage,&LanguagesPage::languagePicked,session,&SessionController::languageChanged);
        connect(languagesPage,&LanguagesPage::backPressed,   this,&MainWindow::goToMain);


        chooseTicketPage=new ChooseTicketPage();
        stack.addWidget(chooseTicketPage);
        connect(chooseTicketPage,&ChooseTicketPage::backPressed ,this,&MainWindow::goToMain);
        connect(chooseTicketPage,&ChooseTicketPage::ticketPicked,session,&SessionController::ticketPicked);
        connect(session,&SessionController::ticketChoiceAccepted,this,&MainWindow::goToInputPersonalDataPage);
        connect(session,&SessionController::ticketChoiceRejected,this,&MainWindow::goToChooseTicketError);

        chooseTicketErrorPage=new ErrorTryAgainPage();
        stack.addWidget(chooseTicketErrorPage);
        connect(chooseTicketErrorPage,&ErrorTryAgainPage::okClicked,this,&MainWindow::goToChooseTicketPage);

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
    //we're passing the language in all 'reinitialize' as an apstraction of translating the page, but it's unused since there is only one language
    void goToMain(){
        mainPage->reinitialize(session->getLanguage());
        stack.setCurrentWidget(mainPage);
    }

    void goToErrorStatePage(ClientShutdownReason reason){
        errorStatePage->reinitialize(reason,session->getLanguage());
        stack.setCurrentWidget(errorStatePage);
    }

    void goToReturningMoneyPage(TransactionCancellationReason reason){
        returningMoneyPage->reinitialize(reason,session->getLanguage());
        stack.setCurrentWidget(returningMoneyPage);
    }

    void goToDebugEditCoins(){
        debugEditCoins->reinitialize(session->getLanguage());
        stack.setCurrentWidget(debugEditCoins);
    }

    void goToLanguages(){
        languagesPage->reinitialize(session->getLanguage());
        stack.setCurrentWidget(languagesPage);
    }

    void goToPrintingPage(){
        printingPage->startPrinting(session->getLanguage());
        stack.setCurrentWidget(printingPage);
    }

    void goToTakeCoinsPage(){
        paymentPage->reinitialize(session->getCurrentTicket(),session->getLanguage());
        session->startTransaction();
        stack.setCurrentWidget(paymentPage);
    }

    void goToInputPersonalDataPage(const TicketData& data){
        inputPersonalDataPage->reinitialize(data,session->getLanguage());
        stack.setCurrentWidget(inputPersonalDataPage);
    }

    void goToChooseTicketError(){
        chooseTicketErrorPage->reinitialize(session->getLanguage());
        session->requestTicketListUpdate();
        stack.setCurrentWidget(chooseTicketErrorPage);
    }

    void goToChooseTicketPage(){
        chooseTicketPage->reinitialize(session->getAvailableTickets(),session->getLanguage());
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