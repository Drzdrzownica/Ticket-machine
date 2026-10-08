#ifndef DEBUGEDITCOINSPAGE_H
#define DEBUGEDITCOINSPAGE_H

#include <QVBoxLayout>
#include <QPushButton>
#include <QHash>
#include <QLineEdit>
#include <QLabel>
#include "client/ClientTypes.h"

class DebugEditCoinsPage:public QWidget{
    Q_OBJECT
    QVBoxLayout layout;

    QPushButton* backButton=new QPushButton("Back",this);
    QHash<Cents,QLineEdit*> denomination_AmountDisplay;
    QHash<Cents,QPushButton*> denomination_SubtractButton;
public:
    DebugEditCoinsPage();
    void reinitialize(Language language);
public slots:
    void setAmountValues(CoinInventory inventory);
signals:
    void backPressed();
    void DEBUGcoinAdded(Cents denomination);
    void DEBUGcoinRemoved(Cents denomination);
};
#endif //DEBUGEDITCOINSPAGE_H