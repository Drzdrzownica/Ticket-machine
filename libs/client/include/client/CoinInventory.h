#ifndef COININVENTORY_H
#define COININVENTORY_H

#include "protocol/constants.h"

struct CoinInventory{
    static constexpr std::array<Cents,8> acceptedDenominationsCents{1,5,10,25,100,500,1000,2000};
    static_assert(std::ranges::is_sorted(acceptedDenominationsCents));
    static_assert(std::ranges::adjacent_find(acceptedDenominationsCents) == acceptedDenominationsCents.end());
    static_assert(acceptedDenominationsCents[0]>0);

    static bool isValidDenomination(Cents value);
    static bool isValidDenomination(const QString& str);
private:
    void validateDenomination(Cents value);
    std::array<quint64,acceptedDenominationsCents.size()> inventory{};
    quint64& amountOf(Cents value);
public:

    void addCoin(Cents value,quint64 amount=1);
    void subtractCoin(Cents value,quint64 amount=1);

    std::vector<std::pair<Cents,quint64>> getInventory()const;
    //we explicitly don't worry about overflow here. There is literally not enough money in the world
    Cents getTotalAmount() const;
    
    quint64 getAmountOfCoins() const;
    auto operator<=>(const CoinInventory&)const=default;
    void moveInventoryFrom(CoinInventory& other);
    void subtractInventory(const CoinInventory& other);
    void addInventory(const CoinInventory& other);

    friend CoinInventory operator+(const CoinInventory& a,const CoinInventory& b);
    std::optional<CoinInventory> coinChange(Cents change) const;
};
#endif //COININVENTORY_H