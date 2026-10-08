#ifndef ERRORTRYAGAINPAGE_G
#define ERRORTRYAGAINPAGE_G

#include <QBoxLayout>
#include <QLabel>
#include <QPushButton>
#include "client/ClientTypes.h"

class ErrorTryAgainPage: public QWidget{
    Q_OBJECT
    QVBoxLayout layout;
    QLabel* errorMessage;
    QPushButton* okButton;
private:
    void disableInput();
    void reEnableInput();
public:
    ErrorTryAgainPage();
    void reinitialize(Language language);
signals:
    void okClicked();
};
#endif //ORTRYAGAINPAGE_G
