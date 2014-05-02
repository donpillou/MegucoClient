
#pragma once

class EBotMarketOrderDraft : public EBotMarketOrder
{
public:
  static const EType eType = EType::botMarketOrderDraft;

public:
  EBotMarketOrderDraft(quint32 id, Type type, const QDateTime& date, double price) : EBotMarketOrder(eType, id)
  {
    this->type = type;
    this->date = date;
    this->price = price;
    this->amount = 0.;
    this->fee = 0.;
    this->total = 0.;
    this->state = State::draft;
  }
};
