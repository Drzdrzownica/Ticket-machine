#ifndef LANGUAGESPAGE_H
#define LANGUAGESPAGE_H

#include <QVBoxLayout>
#include <QPushButton>
#include "client/ClientTypes.h"

class LanguagesPage:public QWidget{
    Q_OBJECT
    QVBoxLayout layout;

    QPushButton* englishBtn;
    QPushButton* backButton;

    void disableInput();
    void reEnableInput();
public:
    LanguagesPage();
    void reinitialize(Language language);
signals:
    void backPressed();
    void languagePicked(Language);
};
#endif //LANGUAGESPAGE_H