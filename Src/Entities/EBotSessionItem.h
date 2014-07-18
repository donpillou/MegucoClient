
#pragma once

class EBotSessionItemDraft;

class EBotSessionItem : public Entity
{
public:
  static const EType eType = EType::botSessionItem;

  enum class Type
  {
    buy = BotProtocol::SessionItem::buy,
    sell = BotProtocol::SessionItem::sell,
  };

  enum class State
  {
    waitBuy = BotProtocol::SessionItem::waitBuy,
    buying = BotProtocol::SessionItem::buying,
    waitSell = BotProtocol::SessionItem::waitSell,
    selling = BotProtocol::SessionItem::selling,
    draft,
  };

public:
  EBotSessionItem(BotProtocol::SessionItem& data) : Entity(eType, data.entityId)
  {
    type = (Type)data.type;
    state = (State)data.state;
    date = QDateTime::fromMSecsSinceEpoch(data.date);
    price = data.price;
    amount = data.amount;
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
  double getAmount() const {return amount;}
  void setAmount(double amount) {this->amount = amount;}
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
  double amount; // >= 0
  double profitablePrice;
  double flipPrice;
  quint32 orderId;
};