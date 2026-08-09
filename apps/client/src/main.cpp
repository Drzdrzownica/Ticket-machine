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
using namespace std;
class LoginPage;
class MainPage;
class LanguagesPage;
class ChooseTicketPage;

class InputPersonalDataPage;

class PaymentPage;
class PriningTicketPage;

//just to look pretty, I won't bother implementing languages, at least I don't think I will.
enum class languages:int{
    english=0
};

struct ticketData{
    QString name;
    quint32 priceCents;
};



//eventually from server, but as it's a prototype...
std::vector<ticketData> createTicketsList(){

    std::vector<ticketData> ans;
    ans.push_back({"lorem",1});
    ans.push_back({"ipsum",2});
    return ans;
}

class MainWindow : public QWidget
{
    std::vector<ticketData> ticketsList;
    LoginPage* loginPage;
    LanguagesPage* languagesPage;
    MainPage* mainPage;

    //the ticket list is from the server and it can change so the page has to be created dynamically
    ChooseTicketPage* chooseTicketPage=nullptr;
    
    
    QVBoxLayout *layout;
    
    languages language=languages::english;
    
    public:
    QStackedWidget *stack;
    
    std::vector<PaymentPage*> paymentPages;
    std::vector<InputPersonalDataPage*> inputPersonalDataPages;
    std::vector<PriningTicketPage*> priningTicketPages;

    MainWindow();
    std::vector<ticketData> getTicketsList() const{
        return ticketsList;
    }
    void loginToDB(const QString& dbID);
    void setLanguage(const languages& lang){
        language=lang;
    };
    void cleanDynamicPages();
    void goToMainPage();
    void goToLangPage();
    void goToChooseTicket();
};

class PaymentPage:public QWidget{
    QVBoxLayout *layout;
    MainWindow* window;
public:
    PaymentPage(MainWindow* Window,ticketData data,QString userName);
};

class InputPersonalDataPage:public QWidget{
    MainWindow* window;
    ticketData data;
    QVBoxLayout *layout;

public:
    InputPersonalDataPage(MainWindow* Window,ticketData Data);
};



class ChooseTicketPage:public QWidget{

    QVBoxLayout *layout;
    MainWindow* window;

    QPushButton* returnToMainPageBtn;

    QLabel* temp;

public:
    ChooseTicketPage(MainWindow* Window):window(Window){

        layout=new QVBoxLayout(this);


        for(const auto& item:window->getTicketsList()){
            QPushButton* btn = new QPushButton(item.name);
            layout->addWidget(btn);
            InputPersonalDataPage* nextPage = new InputPersonalDataPage(window,item);
            window->inputPersonalDataPages.push_back(nextPage);
            window->stack->addWidget(nextPage);
            connect(btn,&QPushButton::pressed,window,[Window,nextPage](){
                Window->stack->setCurrentWidget(nextPage);
            });
        }

        returnToMainPageBtn=new QPushButton("Back");
        layout->addWidget(returnToMainPageBtn);
        connect(returnToMainPageBtn,&QPushButton::pressed,window,&MainWindow::goToMainPage);
    }

};

class LanguagesPage:public QWidget{
    QVBoxLayout *layout;
    QPushButton* englishBtn;

    MainWindow* window;
public:
    LanguagesPage(MainWindow* Window):window(Window){
        layout=new QVBoxLayout(this);

        englishBtn= new QPushButton("English "+QString::fromUcs4(U"\U0001F1FA\U0001F1F8"));

        layout->addWidget(englishBtn);

        connect(englishBtn,&QPushButton::pressed,[this](){
                window->setLanguage(languages::english);
                window->goToMainPage();
            }
        );

    }
};


class LoginPage : public QWidget
{
    QVBoxLayout *layout;

    QLabel *text;
    QPushButton* button;
    QLabel *badLoginText;
    QLineEdit* box;

    MainWindow* window;
    
public:
    LoginPage(MainWindow* Window):window(Window){
        layout=new QVBoxLayout(this);

        text   = new QLabel("Please input the database ID");
        button = new QPushButton("Login");
        badLoginText=new QLabel("ID not found. Try again.");
        box = new QLineEdit;

        badLoginText->setStyleSheet("color:red;");
        badLoginText->setVisible(false);

        layout->addWidget(text,0,Qt::AlignCenter);
        layout->addWidget(box);
        layout->addWidget(badLoginText,0,Qt::AlignCenter);
        layout->addWidget(button);
        
        connect(box,&QLineEdit::returnPressed,button,&QPushButton::click);
        connect(button,&QPushButton::clicked,[this](){
            window->loginToDB(box->text());
            //std::cout<<box->text().toStdString()<<"\n"<<std::flush;
        });
    }
    void loginFailed(){
        badLoginText->setVisible(true);
        box->clear();
    }
    void focusedOn(){
        box->setFocus();
    }
};

class MainPage:public QWidget
{
    QVBoxLayout *layout;

    MainWindow* window;
    QPushButton* languageButton;
    QPushButton* purchaseButton;
    QPushButton* paincButton;



public:
    MainPage(MainWindow* window):window(window){
        layout = new QVBoxLayout(this);

        languageButton = new QPushButton("Language "+QString::fromUcs4(U"\U0001F1FA\U0001F1F8"));
        purchaseButton = new QPushButton("Buy ticket");
        paincButton = new QPushButton("EXIT");

        layout->addWidget(languageButton);
        layout->addWidget(purchaseButton);
        layout->addWidget(paincButton);
        
        connect(paincButton,&QPushButton::clicked,QApplication::quit);
        connect(languageButton,&QPushButton::clicked,window,&MainWindow::goToLangPage);
        connect(purchaseButton,&QPushButton::clicked,window,&MainWindow::goToChooseTicket);
    }
};




class PriningTicketPage:public QWidget{
    QVBoxLayout *layout;
    MainWindow* window;
public:
    PriningTicketPage(MainWindow* window):window(window){
        QVBoxLayout *layout=new QVBoxLayout(this);
        QLabel* printingMessage=new QLabel("printing in progress...");
        layout->addWidget(printingMessage);
    }
};

MainWindow::MainWindow(){
    layout = new QVBoxLayout(this);
    stack=new QStackedWidget;
    layout->addWidget(stack);
    
    loginPage=new LoginPage(this);
    stack->addWidget(loginPage);
    
    mainPage=new MainPage(this);
    stack->addWidget(mainPage);
    
    languagesPage=new LanguagesPage(this);
    stack->addWidget(languagesPage);

    stack->setCurrentWidget(loginPage);
    loginPage->focusedOn();
}

void MainWindow::goToMainPage(){
    cleanDynamicPages();
    stack->setCurrentWidget(mainPage);
}

void MainWindow::loginToDB(const QString& dbID){
    //placeholder
    if(dbID == "temp"){
        ticketsList=createTicketsList();

        stack->setCurrentWidget(mainPage);
    }else{
        loginPage->loginFailed();
    }
}
 
void MainWindow::goToLangPage(){
    cleanDynamicPages();
    stack->setCurrentWidget(languagesPage);
}

void MainWindow::goToChooseTicket(){
    cleanDynamicPages();

    if(chooseTicketPage){
        stack->removeWidget(chooseTicketPage);
        delete chooseTicketPage;
    }

    chooseTicketPage=new ChooseTicketPage(this);
    stack->addWidget(chooseTicketPage);

    stack->setCurrentWidget(chooseTicketPage);
}



void MainWindow::cleanDynamicPages(){
    for(auto& page:paymentPages){
        stack->removeWidget(page);
    }
    paymentPages={};
    for(auto& page:inputPersonalDataPages){
        stack->removeWidget(page);
    }
    inputPersonalDataPages={};
    for(auto& page:priningTicketPages){
        stack->removeWidget(page);
    }
    priningTicketPages={};
}
PaymentPage::PaymentPage(MainWindow* Window,ticketData data,QString userName):window(Window){
    layout = new QVBoxLayout(this);
    //double is a floating point type so this is an error, but will work for now.
    QString instructionText= "Please Insert $"+QString::fromStdString(to_string(((double)data.priceCents)/100));
    QLabel* instruction=new QLabel(instructionText);

    QPushButton* confirmBtn=new QPushButton("Confirm",this);
    QPushButton* cancelBtn=new QPushButton("Done",this);

    layout->addWidget(instruction,0,Qt::AlignCenter);
    layout->addWidget(confirmBtn);
    layout->addWidget(cancelBtn);

    connect(confirmBtn,&QPushButton::clicked,window,[this](){
        
        if(true){//check if inputed number of coins is correct. for now true as a placeholder.

            PriningTicketPage* nextPage=new PriningTicketPage(window);
            window->priningTicketPages.push_back(nextPage);
            window->stack->addWidget(nextPage);
            window->stack->setCurrentWidget(nextPage);
            QTimer::singleShot(3000,this,[this](){
                window->goToMainPage();
            });
        }
    });
    connect(cancelBtn,&QPushButton::clicked,window,&MainWindow::goToMainPage);
}

InputPersonalDataPage::InputPersonalDataPage(MainWindow* Window,ticketData Data):window(Window),data(Data){
    layout = new QVBoxLayout(this);



    QString titleText = QString::fromStdString("Purchasing a ticket for ") + data.name;
    QString instructionText = "Input name to be assigned to the ticket";
    QLabel* title = new QLabel(titleText,this);
    QLabel* instruction = new QLabel(instructionText,this);
    QLineEdit* nameBox=new QLineEdit(this);
    QPushButton* confirmBtn=new QPushButton("Confirm",this);
    QPushButton* cancelBtn=new QPushButton("Cancel",this);
    layout->addWidget(title,0,Qt::AlignCenter);
    layout->addWidget(instruction);
    layout->addWidget(nameBox);
    layout->addWidget(confirmBtn);
    layout->addWidget(cancelBtn);

    connect(confirmBtn,&QPushButton::clicked,window,[this,nameBox](){
        PaymentPage* nextPage= new PaymentPage(window,data,nameBox->text());
        window->paymentPages.push_back(nextPage);
        window->stack->addWidget(nextPage);
        window->stack->setCurrentWidget(nextPage);
    });
    connect(cancelBtn,&QPushButton::clicked,window,&MainWindow::goToMainPage);
}



int main(int argc, char *argv[]){
    QApplication app(argc, argv);
    MainWindow window;
    window.show();

    return app.exec();
}

//outline/plan of a an app that is an apstraction of a vending machine for selling concert tickets. Basically we want something quick and simple to interact with the server and test things. And on personal level I want a milestone I can finish.

//simple main function, just starts the window, nothing fancy

//takes to the login page - apstraction, real one would just connect to the local database, but since we have multiple databases on the same machine we must somehow pick one. (for now databases not yet hooked-up)
//login page takes to a loading screen while establishing connection with the server, and then to the menu page.

//main page - menu. Disabled if there are no tickets avalible according to the server. Buttons, not necesarly in that order:
//ADMIN EXIT - someone using the machine should not be able to turn it off, but since this in an app... (actually I'm not sure about this one, the X in top right should be enough, it's just style)
//ADMIN SHOW_CONTENTS - Show data loaded from the database.
//Language choice - placeholder button, functionally useless because it's too much work
//Buy button

//buy button takes to the list of buttons, one for each avalible tickets

//you input all your data to be printed on the ticket and send to database and then insert coins. the ticket hets printed