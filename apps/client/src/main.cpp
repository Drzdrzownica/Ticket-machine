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

//just to look pretty, I won't bother implementing languages, at least I don't think I will.
enum class Language{
    English=0
};

//stuff like, what coins the machine contains (if it can output exact change), etc. This and all other structs below are essentially placeholders for now.
struct PlaceholderForLocalData{
    int dummyVal=0;
};

struct TicketData{
    using Cents = quint32;
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
    QPushButton languagesBtn{"Languages"};
    QPushButton purchaseBtn{"Buy ticket"};
    QPushButton exitBtn{"EXIT"};

public:
    MainPage():layout(this){
        layout.addWidget(&languagesBtn);
        layout.addWidget(&purchaseBtn);
        layout.addWidget(&exitBtn);
        connect(&languagesBtn,&QPushButton::clicked,this,&MainPage::languagesOption);
        connect(&purchaseBtn,&QPushButton::clicked,this,&MainPage::purchaseOption);
        connect(&exitBtn,&QPushButton::clicked,&QApplication::quit);
    }
signals:
    void languagesOption();
    void purchaseOption();
};


class LanguagesPage:public QWidget{
    Q_OBJECT
    QVBoxLayout layout;

    QPushButton englishBtn{"English "+QString::fromUcs4(U"\U0001F1FA\U0001F1F8")};

public:
    LanguagesPage():layout(this){
        layout.addWidget(&englishBtn);
        connect(&englishBtn,&QPushButton::clicked,this,[this](){emit languagePicked(Language::English);});
    }
signals:
    void languagePicked(Language);
};

class ChooseTicketPage: public QWidget{
    Q_OBJECT
    QVBoxLayout layout;
    QVBoxLayout buttonsLayout;
    
    QLabel errorMessage{"Sorry, no tickets available"};
    QPushButton backButton{"Back"};

    std::vector<QPushButton*> ticketButtons;

    void clearTicketButtons(){
        for(auto* btn:ticketButtons){
            buttonsLayout.removeWidget(btn);
            btn->deleteLater();
        }
        ticketButtons.clear();
    }

public:
    ChooseTicketPage():layout(this){
        layout.addLayout(&buttonsLayout);
        layout.addWidget(&errorMessage);
        layout.addWidget(&backButton);
        connect(&backButton,&QPushButton::clicked,this,&ChooseTicketPage::backPressed);
    }
    void reinitialize(const std::vector<TicketData>& data){
        clearTicketButtons();
        if(data.empty()){
            errorMessage.setVisible(true);
        }else{
            errorMessage.setVisible(false);
            for(auto& ticket:data){
                QPushButton* btn=new QPushButton(ticket.name,this);
                ticketButtons.push_back(btn);
                buttonsLayout.addWidget(btn);
                connect(btn,&QPushButton::clicked,this,[this,ticket](){
                    emit this->ticketPicked(ticket);
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

    QLabel title;
    QLineEdit inputField;
    QPushButton confirmButton{"confirm"};
    QPushButton cancelButton{"Cancel"};


    //placeholder fruction. Will update later
    bool verifyName(){
        if(inputField.text().isEmpty())return false;
        for(auto c:inputField.text()){
            if(c>='a' and c<='z')continue;
            if(c>='A' and c<='Z')continue;
            return false;
        }
        return true;
    }

    void processSubmission(){
        if(verifyName()==false){
            inputField.clear();
        }else{
            FullPurchaseData data{ticketData,inputField.text()};
            emit validPersonalDataSubmitted(data);
        }
    }
public:
    InputPersonalDataPage():layout(this){
        layout.addWidget(&title);
        layout.addWidget(&inputField);
        layout.addWidget(&confirmButton);
        layout.addWidget(&cancelButton);
        connect(&cancelButton,&QPushButton::clicked,this,[this](){
            inputField.setText("");
            emit cancelPressed();
        }
        );
        connect(&confirmButton,&QPushButton::clicked,this,[this](){
            processSubmission();
            inputField.setText("");
        }
        );
    }
    void reinitialize(const TicketData& data){
        ticketData=data;
        title.setText(data.name);
    }
signals:
    void cancelPressed();
    void validPersonalDataSubmitted(FullPurchaseData);
};

class LoginPage: public QWidget{
    Q_OBJECT
    QVBoxLayout layout;
    QLabel instruction{"Enter The DB id"};
    QLineEdit inputBox;
    QLabel tryAgainMessage{"No DB with given ID. Try again"};
    QPushButton loginButton{"login"};

    void loginClicked(){
        //for now placeholder, later try to log into the local db and retrieve real data. /
        //The DB is just local storage, but since this is just a simulation of a real machine, /
        //there can be multiple machines with multiple DBs simulated on a single PC so we have to diferentiate them
        if(inputBox.text()=="temp"){
            PlaceholderForLocalData temp=PlaceholderForLocalData{};
            tryAgainMessage.setVisible(false);
            emit loggedIntoDatabase(temp);
        }else{
            tryAgainMessage.setVisible(true);
        }
    }

public:
    LoginPage():layout(this){
        layout.addWidget(&instruction);
        layout.addWidget(&inputBox);
        layout.addWidget(&tryAgainMessage);
        layout.addWidget(&loginButton);
        tryAgainMessage.setVisible(false);
        connect(&loginButton,&QPushButton::clicked,this,&LoginPage::loginClicked);
    }
signals:
    void loggedIntoDatabase(PlaceholderForLocalData);
};

class PaymentPage: public QWidget{
    Q_OBJECT
    QVBoxLayout layout;
    
    FullPurchaseData data;

    QLabel info{"You are not supposed to see this message"};
    QPushButton confirmButton{"confirm"};
    QPushButton cancelButton{"Cancel"};
    
    //placeholder. For now accept blindly
    void tryAcceptPurchase(){
        if(true){
            emit coinsAccepted(data);
        }
    }

public:
    PaymentPage():layout(this){
        layout.addWidget(&info);
        layout.addWidget(&confirmButton);
        layout.addWidget(&cancelButton);
        connect(&cancelButton,&QPushButton::clicked,this,&PaymentPage::cancelPressed);
        connect(&confirmButton,&QPushButton::clicked,this,&PaymentPage::tryAcceptPurchase);
    }
    void reinitialize(const FullPurchaseData& data){
        this->data=data;
        //for now coins because I don't want to bother with currency yet
        info.setText("insert "+QString::number(data.ticketData.price)+" coins");

    }
signals:
    void cancelPressed();
    void coinsAccepted(FullPurchaseData);
};

class PrintingPage:public QWidget{
    Q_OBJECT
    QVBoxLayout layout;
    QLabel message{"You are not supposed to see this message"};

public:
    PrintingPage():layout(this){
        layout.addWidget(&message);
    }

    //This is obviously an abstarction of a physical process. Real implementation wouldn't need timers. But it will need data to know what we're printing.
    void startPrinting(const FullPurchaseData& data){
        message.setText("Printing in progress...");
        QTimer::singleShot(3000, this, [this] {
            message.setText("Printing Done");
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

    void receiveLanguageChange(const Language& lang){
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
        connect(paymentPage,&PaymentPage::coinsAccepted,this,&MainWindowController::goToPrintingPage);

        inputPersonalDataPage=new InputPersonalDataPage();
        stack.addWidget(inputPersonalDataPage);
        connect(inputPersonalDataPage,&InputPersonalDataPage::cancelPressed,this,&MainWindowController::goToMain);
        connect(inputPersonalDataPage,&InputPersonalDataPage::validPersonalDataSubmitted,this,&MainWindowController::goToTakeCoinsPage);

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
