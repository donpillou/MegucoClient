
#include "stdafx.h"

MarketTradesModel::MarketTradesModel(Entity::Manager& channelEntityManager) :
  channelEntityManager(channelEntityManager),
  upIcon(QIcon(":/Icons/arrow_diag.png")), downIcon(QIcon(":/Icons/arrow_diag_red.png")), neutralIcon(QIcon(":/Icons/bullet_grey.png"))
{
  channelEntityManager.registerListener<EMarketTradeData>(*this);
  eSubscription = channelEntityManager.getEntity<EMarketSubscription>(0);
  Q_ASSERT(eSubscription);
}

MarketTradesModel::~MarketTradesModel()
{
  channelEntityManager.unregisterListener<EMarketTradeData>(*this);

  qDeleteAll(trades);
}

QModelIndex MarketTradesModel::index(int row, int column, const QModelIndex& parent) const
{
  if(hasIndex(row, column, parent))
    return createIndex(row, column, trades.at(row));
  return QModelIndex();
}

QModelIndex MarketTradesModel::parent(const QModelIndex& child) const
{
  return QModelIndex();
}

int MarketTradesModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : trades.size();
}

int MarketTradesModel::columnCount(const QModelIndex& parent) const
{
  return (int)Column::last + 1;
}

QVariant MarketTradesModel::data(const QModelIndex& index, int role) const
{
  const Trade* trade = (const Trade*)index.internalPointer();
  if(!trade)
    return QVariant();

  switch(role)
  {
  case Qt::TextAlignmentRole:
    switch((Column)index.column())
    {
    case Column::price:
    case Column::amount:
      return (int)Qt::AlignRight | (int)Qt::AlignVCenter;
    default:
      return (int)Qt::AlignLeft | (int)Qt::AlignVCenter;
    }
    break;
  case Qt::DecorationRole:
    if((Column)index.column() == Column::price)
      switch(trade->icon)
      {
      case Trade::Icon::up:
        return upIcon;
      case Trade::Icon::down:
        return downIcon;
      case Trade::Icon::neutral:
        return neutralIcon;
      }
    break;
  case Qt::DisplayRole:
    switch((Column)index.column())
    {
    case Column::date:
      {
        quint64 timeSinceTrade = QDateTime::currentDateTime().toTime_t() - trade->date / 1000;
        if(timeSinceTrade < 60)
          //return QString("%1 ms seconds ago").arg(QDateTime::currentMSecsSinceEpoch() - trade->date);
          return QString("a few seconds ago");
        if(timeSinceTrade < 120)
          return QString("1 minute ago");
        return QString("%1 minutes ago").arg(timeSinceTrade / 60);
      }
    case Column::amount:
      return eSubscription->formatAmount(trade->amount);
    case Column::price:
      return eSubscription->formatPrice(trade->price);
    }
  }
  return QVariant();
}

QVariant MarketTradesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(orientation != Qt::Horizontal)
    return QVariant();
  switch(role)
  {
  case Qt::TextAlignmentRole:
    switch((Column)section)
    {
    case Column::price:
    case Column::amount:
      return Qt::AlignRight;
    default:
      return Qt::AlignLeft;
    }
  case Qt::DisplayRole:
    switch((Column)section)
    {
      case Column::date:
        return tr("Date");
      case Column::amount:
        return tr("Amount %1").arg(eSubscription->getCommCurrency());
      case Column::price:
        return tr("Price %1").arg(eSubscription->getBaseCurrency());
    }
  }
  return QVariant();
}

void MarketTradesModel::addedEntity(Entity& entity)
{
  EMarketTradeData* eDataTradeData = dynamic_cast<EMarketTradeData*>(&entity);
  if(eDataTradeData)
  {
    const QList<EMarketTradeData::Trade>& data = eDataTradeData->getData();

    const int totalMaxTrades = 500;

    double lastPrice = 0.;
    int tradesToInsert = data.size();
    if(tradesToInsert == 0)
      return;
    bool isEmpty = trades.isEmpty();
    int maxTrades = isEmpty ? 100 : totalMaxTrades;
    if(tradesToInsert > maxTrades)
    {
      lastPrice = data[tradesToInsert - maxTrades - 1].price;
      tradesToInsert = maxTrades;
    }
    else if(!isEmpty)
      lastPrice = trades.front()->price;
  
    beginInsertRows(QModelIndex(), 0, tradesToInsert - 1);
    trades.reserve(trades.size() + tradesToInsert);
  
    for(int i = data.size() - tradesToInsert, end = data.size(); i < end; ++i)
    {
      const EMarketTradeData::Trade& tradeData = data[i];
      Trade* trade = new Trade;
      trade->date = tradeData.time;
      trade->price = tradeData.price;
      trade->amount = tradeData.amount;
      trade->icon = Trade::Icon::neutral;
      if(lastPrice != 0.)
      {
        if(trade->price > lastPrice)
          trade->icon = Trade::Icon::up;
        else if(trade->price < lastPrice)
          trade->icon = Trade::Icon::down;
      }
      trades.prepend(trade);
      lastPrice = tradeData.price;
    }
    endInsertRows();

    int tradesToRemove = trades.size() - totalMaxTrades;
    if(tradesToRemove > 0)
    {
      beginRemoveRows(QModelIndex(), totalMaxTrades, totalMaxTrades + tradesToRemove - 1);
      for(int i = trades.size() - 1;; --i)
      {
        delete trades.back();
        trades.removeLast();
        if(i == totalMaxTrades)
          break;
      }
      endRemoveRows();
    }

    return;
  }
  Q_ASSERT(false);
}

void MarketTradesModel::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  addedEntity(newEntity);
}

void MarketTradesModel::removedEntity(Entity& entity)
{
}

void MarketTradesModel::removedAll(quint32 type)
{
  if((EType)type == EType::marketTradeData)
  {
    beginResetModel();
    qDeleteAll(trades);
    trades.clear();
    endResetModel();
    return;
  }
  Q_ASSERT(false);
}
