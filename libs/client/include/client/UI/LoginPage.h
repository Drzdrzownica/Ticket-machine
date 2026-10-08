
#ifndef LOGINPAGE_G
#define LOGINPAGE_G

#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

class LoginPage: public QWidget{
    Q_OBJECT
    QVBoxLayout layout;
    QLabel* instruction;
    QLineEdit* inputBox;
    QLabel* tryAgainMessage;
    QPushButton* loginButton;

    void disableInput();
    void reEnableInput();
    void onLoginClicked();

public:
    LoginPage();
public slots:
    void dbIdRejected();
    void allowLogin();
signals:
    void dataBaseIDProvided(QString);
};

#endif //LOGINPAGE_G