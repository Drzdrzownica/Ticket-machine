#ifndef PRINTINGPAGE_H
#define PRINTINGPAGE_H

#include "client/ClientTypes.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>

class PrintingPage:public QWidget{
    Q_OBJECT
    QVBoxLayout layout;
    QLabel* message=new QLabel("You are not supposed to see this message",this);

public:
    PrintingPage();
    //This is obviously an abstraction of a physical process. Real implementation will need data to know what we're printing. Printing introduces edgecases, but we don't worry about that now.
    void startPrinting(Language Language);
signals:
    void printingFinished();
};
#endif //PRINTINGPAGE_H