
#pragma once

class EBotSessionItemDraft;

class EBotSessionItem : public Entity
{
public:
  static const EType eType = EType::botSessionItem;

  enum class Type
  {
    buy = BotProtocol::SessionAsset::buy,
    sell = BotProtocol::SessionAsset::sell,
  };

  enum class State
  {
    waitBuy = BotProtocol::SessionAsset::waitBuy,
    buying = BotProtocol::SessionAsset::buying,
    waitSell = BotProtocol::SessionAsset::waitSell,
    selling = BotProtocol::SessionAsset::selling,
    draft,
  };

public:
  EBotSessionItem(BotProtocol::SessionAsset& data) : Entity(eType, data.entityId)
  {
    type = (Type)data.type;
    state = (State)data.state;
    date = QDateTime::fromMSecsSinceEpoch(data.date);
    price = data.price;
    investComm = data.investComm;
    investBase = data.investBase;
    balanceComm = data.balanceComm;
    balanceBase = data.balanceBase;
    profitablePrice = data.profitablePrice;
    flipPrice = data.flipPrice;
    orderId = data.orderId;
  }
  EBotSessionItem(quint32 id, const EBotSessionItemDraft& sessionItem);

  Type getType() const {return type;}
  State getState() const {return state;}
  void setState(State state) {this->state = state;}
  const QDateTime& getDate() const {return date;}
  double getPrice() const {return price;}
  double getInvestComm() const {return investComm;}
  double getInvestBase() const {return investBase;}
  double getBalanceComm() const {return balanceComm;}
  void setBalanceComm(double balanceComm) {this->balanceComm = balanceComm;}
  double getBalanceBase() const {return balanceBase;}
  void setBalanceBase(double balanceBase) {this->balanceBase = balanceBase;}
  double getProfitablePrice() const {return profitablePrice;}
  double getFlipPrice() const {return flipPrice;}
  void setFlipPrice(double flipPrice) {this->flipPrice = flipPrice;}
  quint32 getOrderId() const {return orderId;}

protected:
  EBotSessionItem(EType type, quint32 id) : Entity(type, id) {}

protected:
  Type type;
  State state;
  QDateTime date;
  double price; // >= 0
  double investComm; // >= 0
  double investBase; // >= 0
  double balanceComm; // >= 0
  double balanceBase; // >= 0
  double profitablePrice;
  double flipPrice;
  quint32 orderId;
};