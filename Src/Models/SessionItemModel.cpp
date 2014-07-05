
#include "stdafx.h"

SessionItemModel::SessionItemModel(Entity::Manager& entityManager) :
  entityManager(entityManager), eSelectedSession(0), draftStr(tr("draft")),
  buyStr(tr("buy")), sellStr(tr("sell")),
  buyingStr(tr("buying...")), sellingStr(tr("selling...")),
  sellIcon(QIcon(":/Icons/money.png")), buyIcon(QIcon(":/Icons/bitcoin.png")),
  dateFormat(QLocale::system().dateTimeFormat(QLocale::ShortFormat))
{
  entityManager.registerListener<EBotSessionItem>(*this);
  entityManager.registerListener<EBotSessionItemDraft>(*this);
  entityManager.registerListener<EBotSession>(*this);
  entityManager.registerListener<EBotService>(*this);
  eBotService = entityManager.getEntity<EBotService>(0);

  eBotMarketAdapter = 0;
}

SessionItemModel::~SessionItemModel()
{
  entityManager.unregisterListener<EBotSessionItem>(*this);
  entityManager.unregisterListener<EBotSessionItemDraft>(*this);
  entityManager.unregisterListener<EBotSession>(*this);
  entityManager.unregisterListener<EBotService>(*this);
}

QModelIndex SessionItemModel::getDraftAmountIndex(EBotSessionItemDraft& draft)
{
  int index = items.indexOf(&draft);
  return createIndex(index, (int)Column::amount, &draft);
}

QModelIndex SessionItemModel::index(int row, int column, const QModelIndex& parent) const
{
  if(hasIndex(row, column, parent))
    return createIndex(row, column, items.at(row));
  return QModelIndex();
}

QModelIndex SessionItemModel::parent(const QModelIndex& child) const
{
  return QModelIndex();
}

int SessionItemModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : items.size();
}

int SessionItemModel::columnCount(const QModelIndex& parent) const
{
  return (int)Column::last + 1;
}

QVariant SessionItemModel::data(const QModelIndex& index, int role) const
{
  const EBotSessionItem* eItem = (const EBotSessionItem*)index.internalPointer();
  if(!eItem)
    return QVariant();

  switch(role)
  {
  case Qt::TextAlignmentRole:
    switch((Column)index.column())
    {
    case Column::price:
    case Column::value:
    case Column::amount:
    case Column::profitablePrice:
    case Column::flipPrice:
      return (int)Qt::AlignRight | (int)Qt::AlignVCenter;
    default:
      return (int)Qt::AlignLeft | (int)Qt::AlignVCenter;
    }
  case Qt::DecorationRole:
    switch((Column)index.column())
    {
    case Column::type:
      switch(eItem->getType())
      {
      case EBotSessionItem::Type::sell:
        return sellIcon;
      case EBotSessionItem::Type::buy:
        return buyIcon;
      default:
        break;
      }
      break;
    case Column::state:
      switch(eItem->getState())
      {
      case EBotSessionItem::State::waitSell:
      case EBotSessionItem::State::selling:
        return sellIcon;
      case EBotSessionItem::State::waitBuy:
      case EBotSessionItem::State::buying:
        return buyIcon;
      case EBotSessionItem::State::draft:
        return eItem->getType() == EBotSessionItem::Type::sell ? sellIcon : buyIcon;
      default:
        break;
      }
      break;
    default:
      break;
    }
    break;
  case Qt::EditRole:
    switch((Column)index.column())
    {
    case Column::amount:
      return eItem->getAmount();
    case Column::flipPrice:
      return eItem->getFlipPrice();
    default:
      break;
    }
    break;
  case Qt::DisplayRole:
    switch((Column)index.column())
    {
    case Column::type:
      switch(eItem->getType())
      {
      case EBotSessionItem::Type::buy:
        return buyStr;
      case EBotSessionItem::Type::sell:
        return sellStr;
      default:
        break;
      }
      break;
    case Column::state:
      switch(eItem->getState())
      {
      case EBotSessionItem::State::waitBuy:
        return buyStr;
      case EBotSessionItem::State::buying:
        return buyingStr;
      case EBotSessionItem::State::waitSell:
        return sellStr;
      case EBotSessionItem::State::selling:
        return sellingStr;
      case EBotSessionItem::State::draft:
        return draftStr;
      default:
        break;
      }
      break;
    case Column::date:
      return eItem->getDate().toString(dateFormat);
    case Column::amount:
      return eBotMarketAdapter->formatAmount(eItem->getAmount());
    case Column::price:
      return eItem->getPrice() == 0 ? QString() : eBotMarketAdapter->formatPrice(eItem->getPrice());
    case Column::value:
      return eBotMarketAdapter->formatPrice(eItem->getAmount() * eItem->getFlipPrice());
    case Column::profitablePrice:
      return eItem->getPrice() == 0 ? QString() : eBotMarketAdapter->formatPrice(eItem->getProfitablePrice());
    case Column::flipPrice:
      return eBotMarketAdapter->formatPrice(eItem->getFlipPrice());
    }
  }
  return QVariant();
}

Qt::ItemFlags SessionItemModel::flags(const QModelIndex &index) const
{
  const EBotSessionItem* eItem = (const EBotSessionItem*)index.internalPointer();
  if(!eItem)
    return 0;

  Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  switch((EBotSessionItem::State)eItem->getState())
  {
  case EBotSessionItem::State::draft:
    {
      Column column = (Column)index.column();
      if(column == Column::amount || column == Column::flipPrice)
        flags |= Qt::ItemIsEditable;
    }
    break;
  case EBotSessionItem::State::waitBuy:
  case EBotSessionItem::State::waitSell:
    {
      Column column = (Column)index.column();
      if(column == Column::flipPrice && eSelectedSession)
        switch(eSelectedSession->getState())
        {
        case EBotSession::State::running:
        case EBotSession::State::simulating:
          flags |= Qt::ItemIsEditable;
          break;
        }
    }
    break;
  default:
    break;
  }
  return flags;
}

bool SessionItemModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
  if (role != Qt::EditRole)
    return false;

  EBotSessionItem* eItem = (EBotSessionItem*)index.internalPointer();
  if(!eItem)
    return false;

  switch(eItem->getState())
  {
  case EBotSessionItem::State::draft:
    switch((Column)index.column())
    {
    case Column::flipPrice:
      {
        double newPrice = value.toDouble();
        if(newPrice <= 0. || newPrice == eItem->getPrice())
          return false;
        if(eItem->getState() == EBotSessionItem::State::draft)
          eItem->setFlipPrice(newPrice);
        return true;
      }
    case Column::amount:
      {
        double newAmount = value.toDouble();
        if(newAmount <= 0. || newAmount == eItem->getAmount())
          return false;
        if(eItem->getState() == EBotSessionItem::State::draft)
          eItem->setAmount(newAmount);
        return true;
      }
    default:
      break;
    }
    break;
  case EBotSessionItem::State::waitBuy:
  case EBotSessionItem::State::waitSell:
    if((Column)index.column() == Column::flipPrice)
    {
      double newFlipPrice = value.toDouble();
      if(newFlipPrice != eItem->getFlipPrice())
        emit editedItemFlipPrice(index, newFlipPrice);
    }
    break;
  default:
    break;
  }

  return false;
}

QVariant SessionItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(orientation != Qt::Horizontal)
    return QVariant();
  switch(role)
  {
  case Qt::TextAlignmentRole:
    switch((Column)section)
    {
    case Column::amount:
    case Column::value:
    case Column::price:
    case Column::profitablePrice:
    case Column::flipPrice:
      return Qt::AlignRight;
    default:
      return Qt::AlignLeft;
    }
  case Qt::DisplayRole:
    switch((Column)section)
    {
      case Column::type:
        return tr("Type");
      case Column::state:
        return tr("State");
      case Column::date:
        return tr("Date");
      case Column::value:
        return tr("Value %1").arg(eBotMarketAdapter ? eBotMarketAdapter->getBaseCurrency() : QString());
      case Column::amount:
        return tr("Amount %1").arg(eBotMarketAdapter ? eBotMarketAdapter->getCommCurrency() : QString());
      case Column::price:
        return tr("Last Price %1").arg(eBotMarketAdapter ? eBotMarketAdapter->getBaseCurrency() : QString());
      case Column::profitablePrice:
        return tr("Min Price %1").arg(eBotMarketAdapter ? eBotMarketAdapter->getBaseCurrency() : QString());
      case Column::flipPrice:
        return tr("Next Price %1").arg(eBotMarketAdapter ? eBotMarketAdapter->getBaseCurrency() : QString());
    }
  }
  return QVariant();
}

void SessionItemModel::addedEntity(Entity& entity)
{
  switch((EType)entity.getType())
  {
  case EType::botSessionItem:
  case EType::botSessionItemDraft:
    {
      EBotSessionItem* eItem = dynamic_cast<EBotSessionItem*>(&entity);
      int index = items.size();
      beginInsertRows(QModelIndex(), index, index);
      items.append(eItem);
      endInsertRows();
      break;
    }
  case EType::botService:
  case EType::botSession:
    break;
  default:
    Q_ASSERT(false);
    break;
  }
}

void SessionItemModel::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  switch((EType)oldEntity.getType())
  {
  case EType::botSessionItem:
  case EType::botSessionItemDraft:
    {
      EBotSessionItem* oldEBotSessionItem = dynamic_cast<EBotSessionItem*>(&oldEntity);
      EBotSessionItem* newEBotSessionItem = dynamic_cast<EBotSessionItem*>(&newEntity);
      int index = items.indexOf(oldEBotSessionItem);
      items[index] = newEBotSessionItem; 
      QModelIndex leftModelIndex = createIndex(index, (int)Column::first, newEBotSessionItem);
      QModelIndex rightModelIndex = createIndex(index, (int)Column::last, newEBotSessionItem);
      emit dataChanged(leftModelIndex, rightModelIndex);
      break;
    }
  case EType::botService:
    eBotService = dynamic_cast<EBotService*>(&newEntity);
    eSelectedSession = eBotService->getSelectedSessionId() != 0 ? entityManager.getEntity<EBotSession>(eBotService->getSelectedSessionId()) : 0;
    break;
  case EType::botSession:
    if(oldEntity.getId() == eBotService->getSelectedSessionId())
      eSelectedSession = dynamic_cast<EBotSession*>(&newEntity);
    break;
  default:
    Q_ASSERT(false);
    break;
  }
}

void SessionItemModel::addedEntity(Entity& entity, Entity& replacedEntity)
{
  updatedEntitiy(replacedEntity, entity);
}

void SessionItemModel::removedEntity(Entity& entity)
{
  switch((EType)entity.getType())
  {
  case EType::botSessionItem:
  case EType::botSessionItemDraft:
    {
      EBotSessionItem* eItem = dynamic_cast<EBotSessionItem*>(&entity);
      int index = items.indexOf(eItem);
      beginRemoveRows(QModelIndex(), index, index);
      items.removeAt(index);
      endRemoveRows();
      break;
    }
  case EType::botService:
    break;
  case EType::botSession:
    if(entity.getId() == eBotService->getSelectedSessionId())
      eSelectedSession = 0;
    break;
  default:
    Q_ASSERT(false);
    break;
  }
}

void SessionItemModel::removedAll(quint32 type)
{
  switch((EType)type)
  {
  case EType::botSessionItem:
  case EType::botSessionItemDraft:
    emit beginResetModel();
    items.clear();
    emit endResetModel();
    break;;
  case EType::botService:
  case EType::botSession:
    break;
  default:
    Q_ASSERT(false);
    break;
  }
}
