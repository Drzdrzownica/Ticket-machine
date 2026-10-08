#include "client/UI/ErrorStatePage.h"


ErrorStatePage::ErrorStatePage():layout(this){
    information=new QLabel("Something went seriously wrong contact the administrator",this);
    details=new QLabel("This page has not been initialized with a shutdown reason",this);
    layout.addWidget(information);
    layout.addWidget(details);
}

void ErrorStatePage::reinitialize(ClientShutdownReason reason,Language language){
    details->setText("Shutdown reason code "+QString::number(static_cast<std::underlying_type_t<ClientShutdownReason>>(reason))+".");
    if(reason==ClientShutdownReason::Socket_error)details->setText(details->text()+"\nServer is likely offline. Make sure it's online and reset the application.");
}