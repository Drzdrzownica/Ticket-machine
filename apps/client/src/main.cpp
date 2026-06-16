#include <QApplication>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QPushButton>
#include <QRandomGenerator>
#include <QTcpSocket>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>


// widgets.h

class MainWindow;





struct TicketData{
    QString name;
    //will add more later
};







class LoginPage : public QWidget
{
public:
    LoginPage(MainWindow *window);
    void tryLogin();

private:
    QLabel * m_text;
    QLineEdit *inputBox;
    QPushButton *btnLogin;
    QLabel* invalidDbError;
    QPushButton* btnExit;

    MainWindow* m_window;
};








class MainPage : public QWidget
{
public:
    MainPage(MainWindow *window);
    void addButton(QString);
private:
    QList<QPushButton*> optionButtons;
    QPushButton* btnDataBase;
    QPushButton* btnExit;

    QVBoxLayout *layout;

    MainWindow* m_window;
};






class ConnectionErrorPage : public QWidget
{
public:
    ConnectionErrorPage();

private:
    QLabel * m_text;
    QPushButton* btnExit;
};








class MainWindow : public QWidget
{
public:
    MainWindow();
    void gotoMainPage();
    void gotoConnectionError();
    void handleTcpData(QString);
    QStackedWidget *m_stack;
    
    LoginPage *m_loginPage;
    MainPage *m_mainPage;
    ConnectionErrorPage* m_connectionError;
    QTcpSocket socket;
};
















// widgets.cpp








void MainWindow::gotoMainPage()
{
    m_stack->setCurrentWidget(m_mainPage);
}


void MainWindow::gotoConnectionError(){
    m_stack->setCurrentWidget(m_connectionError);
}

MainWindow::MainWindow()
{
    m_stack = new QStackedWidget(this);

    m_loginPage = new LoginPage(this);
    m_mainPage = new MainPage(this);
    m_connectionError = new ConnectionErrorPage();
    
    m_stack->addWidget(m_loginPage);
    m_stack->addWidget(m_mainPage);
    m_stack->addWidget(m_connectionError);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_stack);


    connect(&socket,&QTcpSocket::disconnected,this,&MainWindow::gotoConnectionError);
    connect(&socket,&QTcpSocket::errorOccurred,this,&MainWindow::gotoConnectionError);

    connect(&socket,&QTcpSocket::readyRead,[this](){
        handleTcpData(QString::fromUtf8(socket.readAll()).trimmed());
    });

    connect(&socket, &QTcpSocket::connected, this, [this]() {
        socket.write("BTN");
    });

    socket.connectToHost(QHostAddress::LocalHost, 12345);
}

void MainWindow::handleTcpData(QString s){
    if(s.startsWith("BTN")){
        m_mainPage->addButton(s.mid(4));
    }
}

void LoginPage::tryLogin(){
    auto text = inputBox->text();
    if(text=="todo"){
        invalidDbError->setStyleSheet("color: transparent;");
        m_window->gotoMainPage();
    }else {
        invalidDbError->setStyleSheet("color: red;");
    }
}

LoginPage::LoginPage(MainWindow *window): QWidget(), m_window(window)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    m_text = new QLabel("Enter your Database ID");
    inputBox = new QLineEdit();
    btnLogin = new QPushButton("Log In");
    invalidDbError = new QLabel();
    btnExit = new QPushButton("Exit");
    
    layout->addWidget(m_text);
    layout->addWidget(inputBox);
    layout->addWidget(invalidDbError);
    layout->addWidget(btnLogin);
    layout->addWidget(btnExit);

    invalidDbError->setText("*type in valid db ID");
    invalidDbError->setStyleSheet("color: transparent;");

    inputBox->setMaxLength(10);

    connect(btnLogin, &QPushButton::clicked, this, &LoginPage::tryLogin);
    connect(inputBox, &QLineEdit::returnPressed, this, &LoginPage::tryLogin);
    connect(btnExit, &QPushButton::clicked, qApp, &QApplication::quit);

}

MainPage::MainPage(MainWindow *window): QWidget(), m_window(window)
{

    layout = new QVBoxLayout(this);
    
    btnDataBase = new QPushButton("ADMIN display contents");
    btnExit = new QPushButton("Exit");

    layout->addWidget(btnDataBase);
    layout->addWidget(btnExit);

    connect(btnExit, &QPushButton::clicked, qApp, &QApplication::quit);

}

void MainPage::addButton(QString s){
    QPushButton* btn = new QPushButton(s);
    layout->addWidget(btn);
    optionButtons.append(btn);
}


ConnectionErrorPage::ConnectionErrorPage()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    m_text=new QLabel();
    m_text->setText("Connection error. Please restart the application.");
    btnExit=new QPushButton("Exit");

    layout->addWidget(m_text);
    layout->addWidget(btnExit);

    connect(btnExit, &QPushButton::clicked, qApp, &QApplication::quit);
}













//main.cpp

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();
}