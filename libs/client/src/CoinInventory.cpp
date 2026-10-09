#include "client/CoinInventory.h"

bool CoinInventory::isValidDenomination(Cents value){
    return std::find(acceptedDenominationsCents.begin(),acceptedDenominationsCents.end(),value)!=acceptedDenominationsCents.end();
}
bool CoinInventory::isValidDenomination(const QString& str){
    bool isNum;
    quint64 value=str.toULongLong(&isNum);
    if(!isNum)return false;
    return isValidDenomination(value);
}

void CoinInventory::validateDenomination(Cents value){
    if(!isValidDenomination(value))throw std::invalid_argument("not a valid denomination");
}
quint64& CoinInventory::amountOf(Cents value){
    //the competitive programmer living in my heart screams at me to optimize those O(n) look-ups, but it's really unnecessary here.
    auto iterator=std::find(acceptedDenominationsCents.begin(),acceptedDenominationsCents.end(),value);
    if(iterator==acceptedDenominationsCents.end())throw std::invalid_argument("Not a denomination");
    return inventory[std::distance(acceptedDenominationsCents.begin(),iterator)];
}
void CoinInventory::addCoin(Cents value,quint64 amount){
    validateDenomination(value);
    amountOf(value)+=amount;
}
void CoinInventory::subtractCoin(Cents value,quint64 amount){
    validateDenomination(value);
    if(amountOf(value)<amount)throw std::underflow_error("No coins to subtract");
    amountOf(value)-=amount;
}

std::vector<std::pair<Cents,quint64>> CoinInventory::getInventory()const{
    std::vector<std::pair<Cents,quint64>> result;
    result.reserve(inventory.size());
    for(std::size_t i=0;i<inventory.size();i++){
        result.push_back({acceptedDenominationsCents[i],inventory[i]});
    }
    return result;
}
//we explicitly don't worry about overflow here. There is literally not enough money in the world
Cents CoinInventory::getTotalAmount() const{
    Cents result=0;
    for(std::size_t i=0;i<inventory.size();i++){
        result+=inventory[i]*acceptedDenominationsCents[i];
    }
    return result;
}

quint64 CoinInventory::getAmountOfCoins() const{
    quint64 result=0;
    for(quint64 amount:inventory)result+=amount;
    return result;
}

void CoinInventory::moveInventoryFrom(CoinInventory& other){
    if(this==&other)throw std::invalid_argument("move to itself is invalid");
    for(std::size_t i=0;i<inventory.size();i++){
        inventory[i]+=other.inventory[i];
        other.inventory[i]=0;
    }
}
void CoinInventory::subtractInventory(const CoinInventory& other){
    for(std::size_t i=0;i<inventory.size();i++){
        if(inventory[i]<other.inventory[i])throw std::underflow_error("No coins to subtract");
    }
    for(std::size_t i=0;i<inventory.size();i++){
        inventory[i]-=other.inventory[i];
    }
}
void CoinInventory::addInventory(const CoinInventory& other){
    for(std::size_t i=0;i<inventory.size();i++){
        inventory[i]+=other.inventory[i];
    }
}

CoinInventory operator+(const CoinInventory& a,const CoinInventory& b){
    CoinInventory result{};
    result.addInventory(a);
    result.addInventory(b);
    return result;
}

std::optional<CoinInventory> CoinInventory::coinChange(Cents change) const{
    static constexpr Cents maximumChange=50000; //500$ protection agains abuse, since dp can theoreticaly consume a lot of time and memory. If someone inserts that much money, he's not serious. We can safely reject it.
    static constexpr Cents maximumCoinsDispensed=100; //protection against expensive calculations, and we don't want to flood user with too many coins anyway.
    if(change==0)return CoinInventory{};
    if(change>maximumChange)return {}; 
    CoinInventory boundedInventory=*this;
    for(auto& count:boundedInventory.inventory)count=std::min(count,maximumCoinsDispensed); 

    std::vector<std::optional<CoinInventory>> oldDp(change+1);
    std::vector<std::optional<CoinInventory>> dp(change+1);
    dp[0]=CoinInventory{};
    for(int coinIndex=acceptedDenominationsCents.size()-1;coinIndex>=0;coinIndex--){
        oldDp=dp;
        for(quint64 coinAmount=1;coinAmount<=boundedInventory.inventory[coinIndex];coinAmount++){
            quint64 groupValue=coinAmount*acceptedDenominationsCents[coinIndex];
            for(size_t i=groupValue;i<dp.size();i++){
                auto& previous = oldDp[i - groupValue];
                if(!previous)continue;
                const quint64 potentialCoinCount=previous->getAmountOfCoins() + coinAmount;
                if(potentialCoinCount>maximumCoinsDispensed)continue;
                if(!dp[i] or (dp[i]->getAmountOfCoins()>potentialCoinCount)){
                    dp[i]=previous;
                    dp[i]->inventory[coinIndex]+=coinAmount;
                }
            }
        }
    }
    return dp[change];
}