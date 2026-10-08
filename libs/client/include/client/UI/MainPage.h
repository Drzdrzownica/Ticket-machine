
#ifndef MAINPAGE_H
#define MAINPAGE_H

#include "client/ClientTypes.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QWidget>
#include <QApplication>

class MainPage:public QWidget{
    Q_OBJECT
    QVBoxLayout layout;
    //goes without saying this is only for this version of the app, conceptually it is not here, or wouldn't be in the production
    QPushButton* debugEditCoinsBtn;
    //languages are a non-functional placeholde (since a page with a single option seems redundant,and I still want to have a main page)
    QPushButton* languagesBtn;
    QPushButton* purchaseBtn;
    QPushButton* exitBtn;

    void disableInput();
    void reEnableInput();
public:
    MainPage();
    void reinitialize(Language language);
signals:
    void debugEditCoinsOption();
    void languagesOption();
    void purchaseOption();
};
#endif //MAINPAGE_H