
#include "client/UI/DebugEditCoinsPage.h"

DebugEditCoinsPage::DebugEditCoinsPage():layout(this){
    for(Cents denominationVal:CoinInventory::acceptedDenominationsCents){
        QHBoxLayout* rowLayout=new QHBoxLayout;
        QString denominationStr=QString::number(denominationVal);
        QLabel* denominationLabel=new QLabel(denominationStr);
        QLineEdit* currentValue=new QLineEdit("0");
        currentValue->setReadOnly(true);
        currentValue->setFocusPolicy(Qt::NoFocus);
        currentValue->setFixedWidth(50);
        QPushButton* subBtn=new QPushButton("-");
        subBtn->setFixedWidth(50);
        QPushButton* addBtn=new QPushButton("+");
        addBtn->setFixedWidth(50);

        connect(subBtn,&QPushButton::clicked,this,[this,denominationVal](){emit DEBUGcoinRemoved(denominationVal);});
        connect(addBtn,&QPushButton::clicked,this,[this,denominationVal](){emit DEBUGcoinAdded(denominationVal);});

        denomination_AmountDisplay[denominationVal]=currentValue;
        denomination_SubtractButton[denominationVal]=subBtn;
        subBtn->setDisabled(true);

        rowLayout->addWidget(denominationLabel);
        rowLayout->addStretch();
        rowLayout->addWidget(currentValue);
        rowLayout->addWidget(subBtn);
        rowLayout->addWidget(addBtn);

        layout.addLayout(rowLayout);
    }
    connect(backButton,&QPushButton::clicked,this,&DebugEditCoinsPage::backPressed);
    layout.addWidget(backButton);
}
void DebugEditCoinsPage::reinitialize(Language language){}
void DebugEditCoinsPage::setAmountValues(CoinInventory inventory){
    for(const auto& [denomination,amount]:inventory.getInventory()){
        denomination_AmountDisplay[denomination]->setText(QString::number(amount));
        QPushButton* subBtn=denomination_SubtractButton[denomination];
        if(amount==0)subBtn->setDisabled(true);
        else subBtn->setDisabled(false);
    }
}