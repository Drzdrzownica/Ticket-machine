
#include "client/UI/PrintingPage.h"

PrintingPage::PrintingPage():layout(this){
    layout.addWidget(message);
}

//This is obviously an abstraction of a physical process. Real implementation will need data to know what we're printing. Printing introduces edgecases, but we don't worry about that now.
void PrintingPage::startPrinting(Language Language){
    message->setText("Printing in progress...");
    QTimer::singleShot(3000, this, [this] {
        message->setText("Printing Done");
        QTimer::singleShot(2000, this, [this] {
            emit printingFinished();
        });
    });
}