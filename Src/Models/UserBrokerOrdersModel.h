
#pragma once

class UserBrokerOrdersModel : public QAbstractItemModel, public Entity::Listener
{
  Q_OBJECT

public:
  enum class Column
  {
      first,
      type = first,
      state,
      date,
      value,
      amount,
      price,
      total,
      last = total,
  };

public:
  UserBrokerOrdersModel(Entity::Manager& entityManager);
  ~UserBrokerOrdersModel();

  QModelIndex getDraftAmountIndex(EUserBrokerOrderDraft& draft);

signals:
  void editedOrderPrice(const QModelIndex& index, double price);
  void editedOrderAmount(const QModelIndex& index, double price);

private:
  Entity::Manager& entityManager;
  EBrokerType* eBrokerType;
  QList<EUserBrokerOrder*> orders;
  QVariant draftStr;
  QVariant submittingStr;
  QVariant updatingStr;
  QVariant openStr;
  QVariant cancelingStr;
  QVariant canceledStr;
  QVariant closedStr;
  QVariant errorStr;
  QVariant buyStr;
  QVariant sellStr;
  QVariant sellIcon;
  QVariant buyIcon;
  QString dateFormat;

private: // QAbstractItemModel
  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
  virtual QModelIndex parent(const QModelIndex& child) const;
  virtual int rowCount(const QModelIndex& parent) const;
  virtual int columnCount(const QModelIndex& parent) const;
  virtual QVariant data(const QModelIndex& index, int role) const;
  virtual Qt::ItemFlags flags(const QModelIndex &index) const;
  virtual bool setData(const QModelIndex & index, const QVariant & value, int role);
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private: // Entity::Listener
  virtual void addedEntity(Entity& entity);
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);
  virtual void addedEntity(Entity& entity, Entity& replacedEntity);
  virtual void removedEntity(Entity& entity);
  virtual void removedAll(quint32 type);
};

class UserBrokerOrdersSortProxyModel : public QSortFilterProxyModel
{
public:
  UserBrokerOrdersSortProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {}

private:
  virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const
  {
    const EUserSessionOrder* leftOrder = (const EUserSessionOrder*)left.internalPointer();
    const EUserSessionOrder* rightOrder = (const EUserSessionOrder*)right.internalPointer();
    switch((UserBrokerOrdersModel::Column)left.column())
    {
    case UserBrokerOrdersModel::Column::date:
      return leftOrder->getDate().msecsTo(rightOrder->getDate()) > 0;
    case UserBrokerOrdersModel::Column::value:
      return leftOrder->getAmount() * leftOrder->getPrice() < rightOrder->getAmount() * rightOrder->getPrice();
    case UserBrokerOrdersModel::Column::amount:
      return leftOrder->getAmount() < rightOrder->getAmount();
    case UserBrokerOrdersModel::Column::price:
      return leftOrder->getPrice() < rightOrder->getPrice();
    case UserBrokerOrdersModel::Column::total:
      return leftOrder->getTotal() < rightOrder->getTotal();
    default:
      break;
    }
    return QSortFilterProxyModel::lessThan(left, right);
  }
};
