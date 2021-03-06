
#include "stdafx.h"

UserSessionTransactionsModel::UserSessionTransactionsModel(Entity::Manager& entityManager) :
  entityManager(entityManager),
  buyStr(tr("buy")), sellStr(tr("sell")),
  sellIcon(QIcon(":/Icons/money.png")), buyIcon(QIcon(":/Icons/bitcoin.png")),
  dateFormat(QLocale::system().dateTimeFormat(QLocale::ShortFormat))
{
  entityManager.registerListener<EUserSessionTransaction>(*this);
  entityManager.registerListener<EConnection>(*this);

  eBrokerType = 0;
}

UserSessionTransactionsModel::~UserSessionTransactionsModel()
{
  entityManager.unregisterListener<EUserSessionTransaction>(*this);
  entityManager.unregisterListener<EConnection>(*this);
}

QModelIndex UserSessionTransactionsModel::index(int row, int column, const QModelIndex& parent) const
{
  if(hasIndex(row, column, parent))
    return createIndex(row, column, transactions.at(row));
  return QModelIndex();
}

QModelIndex UserSessionTransactionsModel::parent(const QModelIndex& child) const
{
  return QModelIndex();
}

int UserSessionTransactionsModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : transactions.size();
}

int UserSessionTransactionsModel::columnCount(const QModelIndex& parent) const
{
  return (int)Column::last + 1;
}

QVariant UserSessionTransactionsModel::data(const QModelIndex& index, int role) const
{
  const EUserSessionTransaction* eTransaction = (const EUserSessionTransaction*)index.internalPointer();
  if(!eTransaction)
    return QVariant();

  switch(role)
  {
  case  Qt::TextAlignmentRole:
    switch((Column)index.column())
    {
    case Column::price:
    case Column::value:
    case Column::amount:
    case Column::fee:
    case Column::total:
      return (int)Qt::AlignRight | (int)Qt::AlignVCenter;
    default:
      return (int)Qt::AlignLeft | (int)Qt::AlignVCenter;
    }
  case Qt::DecorationRole:
    if((Column)index.column() == Column::type)
      switch(eTransaction->getType())
      {
      case EUserSessionTransaction::Type::sell:
        return sellIcon;
      case EUserSessionTransaction::Type::buy:
        return buyIcon;
      default:
        break;
      }
    break;
  case Qt::DisplayRole:
    switch((Column)index.column())
    {
    case Column::type:
      switch(eTransaction->getType())
      {
      case EUserSessionTransaction::Type::buy:
        return buyStr;
      case EUserSessionTransaction::Type::sell:
        return sellStr;
      default:
        break;
      }
    case Column::date:
      return eTransaction->getDate().toString(dateFormat);
    case Column::amount:
      return eBrokerType->formatAmount(eTransaction->getAmount());
    case Column::price:
      return eBrokerType->formatPrice(eTransaction->getPrice());
    case Column::value:
      return eBrokerType->formatPrice(eTransaction->getAmount() * eTransaction->getPrice());
    case Column::fee:
      return eBrokerType->formatPrice(eTransaction->getFee());
    case Column::total:
      return eTransaction->getType() == EUserSessionTransaction::Type::sell ? (QString("+") + eBrokerType->formatPrice(eTransaction->getTotal())) : eBrokerType->formatPrice(-eTransaction->getTotal());
    }
  }
  return QVariant();
}

QVariant UserSessionTransactionsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(orientation != Qt::Horizontal)
    return QVariant();
  switch(role)
  {
  case Qt::TextAlignmentRole:
    switch((Column)section)
    {
    case Column::price:
    case Column::value:
    case Column::amount:
    case Column::fee:
    case Column::total:
      return Qt::AlignRight;
    default:
      return Qt::AlignLeft;
    }
  case Qt::DisplayRole:
    switch((Column)section)
    {
      case Column::type:
        return tr("Type");
      case Column::date:
        return tr("Date");
      case Column::amount:
        return tr("Amount %1").arg(eBrokerType ? eBrokerType->getCommCurrency() : QString());
      case Column::price:
        return tr("Price %1").arg(eBrokerType ? eBrokerType->getBaseCurrency() : QString());
      case Column::value:
        return tr("Value %1").arg(eBrokerType ? eBrokerType->getBaseCurrency() : QString());
      case Column::fee:
        return tr("Fee %1").arg(eBrokerType ? eBrokerType->getBaseCurrency() : QString());
      case Column::total:
        return tr("Total %1").arg(eBrokerType ? eBrokerType->getBaseCurrency() : QString());
    }
  }
  return QVariant();
}

void UserSessionTransactionsModel::addedEntity(Entity& entity)
{
  switch((EType)entity.getType())
  {
  case EType::userSessionTransaction:
    {
      EUserSessionTransaction* eTransaction = dynamic_cast<EUserSessionTransaction*>(&entity);
      int index = transactions.size();
      beginInsertRows(QModelIndex(), index, index);
      transactions.append(eTransaction);
      endInsertRows();
      break;
    }
  case EType::connection:
    break;
  default:
    Q_ASSERT(false);
    break;
  }
}

void UserSessionTransactionsModel::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  switch((EType)oldEntity.getType())
  {
  case EType::userSessionTransaction:
    {
      EUserSessionTransaction* oldEBotSessionTransaction = dynamic_cast<EUserSessionTransaction*>(&oldEntity);
      EUserSessionTransaction* newEBotSessionTransaction = dynamic_cast<EUserSessionTransaction*>(&newEntity);
      int index = transactions.indexOf(oldEBotSessionTransaction);
      transactions[index] = newEBotSessionTransaction; 
      QModelIndex leftModelIndex = createIndex(index, (int)Column::first, newEBotSessionTransaction);
      QModelIndex rightModelIndex = createIndex(index, (int)Column::last, newEBotSessionTransaction);
      emit dataChanged(leftModelIndex, rightModelIndex);
      break;
    }
  case EType::connection:
    {
      EConnection* eDataService = dynamic_cast<EConnection*>(&newEntity);
      EBrokerType* newBrokerType = 0;
      if(eDataService && eDataService->getSelectedSessionId() != 0)
      {
        EUserSession* eSession = entityManager.getEntity<EUserSession>(eDataService->getSelectedSessionId());
        if(eSession && eSession->getBrokerId() != 0)
        {
          EUserBroker* eUserBroker = entityManager.getEntity<EUserBroker>(eSession->getBrokerId());
          if(eUserBroker && eUserBroker->getBrokerTypeId() != 0)
            newBrokerType = entityManager.getEntity<EBrokerType>(eUserBroker->getBrokerTypeId());
        }
      }
      if(newBrokerType != eBrokerType)
      {
        eBrokerType = newBrokerType;
        headerDataChanged(Qt::Horizontal, (int)Column::first, (int)Column::last);
      }
      break;
    }
  default:
    Q_ASSERT(false);
    break;
  }
}

void UserSessionTransactionsModel::removedEntity(Entity& entity)
{
  switch((EType)entity.getType())
  {
  case EType::userSessionTransaction:
    {
      EUserSessionTransaction* eTransaction = dynamic_cast<EUserSessionTransaction*>(&entity);
      int index = transactions.indexOf(eTransaction);
      beginRemoveRows(QModelIndex(), index, index);
      transactions.removeAt(index);
      endRemoveRows();
      break;
    }
  case EType::connection:
    break;
  default:
    Q_ASSERT(false);
    break;
  }
}

void UserSessionTransactionsModel::removedAll(quint32 type)
{
  switch((EType)type)
  {
  case EType::userSessionTransaction:
    if(!transactions.isEmpty())
    {
      emit beginResetModel();
      transactions.clear();
      emit endResetModel();
    }
    break;
  case EType::connection:
    break;
  default:
    Q_ASSERT(false);
    break;
  }
}
