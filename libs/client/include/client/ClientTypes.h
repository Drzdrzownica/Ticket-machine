#ifndef CIENTTYPES_H
#define CIENTTYPES_H

#include "protocol/constants.h"

//just to look pretty, I won't bother implementing languages, at least I don't think I will.
enum class Language{
    English
};

enum class TransactionCancellationReason{
    CouldNotGiveOutChange,
    ServerDisconnected,
    UserRequested,
    SocketError,
    BuyFailed,
    Error
};

enum class ClientShutdownReason:quint64{
    Version_validation_mismatch,
    Version_validation_issue,
    Ticket_list_parsing_issue,
    Ticket_list_retrieval_issue,
    Generic_server_requested,
    Response_parsing_issue,
    Unexpected_response_type,
    Transaction_state_violation,
    Server_disconnected,
    Socket_error
};

struct TicketData{
    TicketId Id; //alias of an integral type defined in constants.h for compatibility with server
    QString name;
    Cents price; //same as above
    bool isAvailable=true;
};

struct CoinInventory{
    static constexpr std::array<Cents,8> acceptedDenominationsCents{1,5,10,25,100,500,1000,2000};
    static_assert(std::ranges::is_sorted(acceptedDenominationsCents));
    static_assert(std::ranges::adjacent_find(acceptedDenominationsCents) == acceptedDenominationsCents.end());
    static_assert(acceptedDenominationsCents[0]>0);
    static bool isValidDenomination(Cents value){
        return std::find(acceptedDenominationsCents.begin(),acceptedDenominationsCents.end(),value)!=acceptedDenominationsCents.end();
    }
    static bool isValidDenomination(const QString& str){
        bool isNum;
        quint64 value=str.toULongLong(&isNum);
        if(!isNum)return false;
        return isValidDenomination(value);
    }
private:
    std::array<quint64,acceptedDenominationsCents.size()> inventory{};
    quint64& amountOf(Cents value){
        //the competitive programmer living in my heart screams at me to optimize those O(n) look-ups, but it's really unnecessary here.
        auto iterator=std::find(acceptedDenominationsCents.begin(),acceptedDenominationsCents.end(),value);
        Q_ASSERT(iterator!=acceptedDenominationsCents.end());
        return inventory[std::distance(acceptedDenominationsCents.begin(),iterator)];
    }
public:

    void addCoin(Cents value,quint64 amount=1){
        Q_ASSERT(isValidDenomination(value));
        amountOf(value)+=amount;
    }
    void subtractCoin(Cents value,quint64 amount=1){
        Q_ASSERT(isValidDenomination(value));
        Q_ASSERT(amountOf(value)>=amount);
        amountOf(value)-=amount;
    }

    std::vector<std::pair<Cents,quint64>> getInventory()const{
        std::vector<std::pair<Cents,quint64>> result;
        result.reserve(inventory.size());
        for(std::size_t i=0;i<inventory.size();i++){
            result.push_back({acceptedDenominationsCents[i],inventory[i]});
        }
        return result;
    }
    //we explicitly don't worry about overflow here. There is literally not enough money in the world
    Cents getTotalAmount() const{
        Cents result=0;
        for(std::size_t i=0;i<inventory.size();i++){
            result+=inventory[i]*acceptedDenominationsCents[i];
        }
        return result;
    }
    
    quint64 getAmountOfCoins() const{
        quint64 result=0;
        for(quint64 amount:inventory)result+=amount;
        return result;
    }

    auto operator<=>(const CoinInventory&)const=default;
    void moveInventoryFrom(CoinInventory& other){
        Q_ASSERT(this!=&other);
        for(std::size_t i=0;i<inventory.size();i++){
            inventory[i]+=other.inventory[i];
            other.inventory[i]=0;
        }
    }
    void subtractInventory(const CoinInventory& other){
        for(std::size_t i=0;i<inventory.size();i++){
            Q_ASSERT(inventory[i]>=other.inventory[i]);
            inventory[i]-=other.inventory[i];
        }
    }
    void addInventory(const CoinInventory& other){
        for(std::size_t i=0;i<inventory.size();i++){
            inventory[i]+=other.inventory[i];
        }
    }

    friend CoinInventory operator+(const CoinInventory& a,const CoinInventory& b){
        CoinInventory result{};
        result.addInventory(a);
        result.addInventory(b);
        return result;
    }

    std::optional<CoinInventory> coinChange(Cents change) const{
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
};

#endif //CIENTTYPES_H