
#include "stdafx.h"

UserSessionLogModel::UserSessionLogModel(Entity::Manager& entityManager) :
  entityManager(entityManager),
  informationIcon(QIcon(":/Icons/information.png")),
  dateFormat(QLocale::system().dateTimeFormat(QLocale::ShortFormat))
{
  entityManager.registerListener<EUserSessionLogMessage>(*this);
}

UserSessionLogModel::~UserSessionLogModel()
{
  entityManager.unregisterListener<EUserSessionLogMessage>(*this);

  qDeleteAll(messages);
}

QModelIndex UserSessionLogModel::index(int row, int column, const QModelIndex& parent) const
{
  if(hasIndex(row, column, parent))
    return createIndex(row, column, messages.at(row));
  return QModelIndex();
}

QModelIndex UserSessionLogModel::parent(const QModelIndex& child) const
{
  return QModelIndex();
}

int UserSessionLogModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : messages.size();
}

int UserSessionLogModel::columnCount(const QModelIndex& parent) const
{
  return (int)Column::last + 1;
}

QVariant UserSessionLogModel::data(const QModelIndex& index, int role) const
{
  const Item* item = (const Item*)index.internalPointer();
  if(!item)
    return QVariant();

  switch(role)
  {
  case Qt::DecorationRole:
    if((Column)index.column() == Column::message)
      return informationIcon;
    break;
  case Qt::DisplayRole:
    switch((Column)index.column())
    {
    case Column::date:
      return item->date.toString(dateFormat);
    case Column::message:
      return item->message;
    }
  }
  return QVariant();
}

QVariant UserSessionLogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(orientation != Qt::Horizontal)
    return QVariant();
  switch(role)
  {
  case Qt::DisplayRole:
    switch((Column)section)
    {
    case Column::date:
      return tr("Date");
    case Column::message:
      return tr("Message");
    default:
      break;
    }
  }
  return QVariant();
}

void UserSessionLogModel::addedEntity(Entity& entity)
{
  EUserSessionLogMessage* eSessionLogMessage = dynamic_cast<EUserSessionLogMessage*>(&entity);
  if(eSessionLogMessage)
  {
    int index = messages.size();
    Item* item = new Item;
    item->date = eSessionLogMessage->getDate();
    item->message = eSessionLogMessage->getMessage();
    beginInsertRows(QModelIndex(), index, index);
    messages.append(item);
    endInsertRows();
    return;
  }
  Q_ASSERT(false);
}

void UserSessionLogModel::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  addedEntity(newEntity);
}

void UserSessionLogModel::removedAll(quint32 type)
{
  if((EType)type == EType::userSessionLogMessage)
  {
    emit beginResetModel();
    qDeleteAll(messages);
    messages.clear();
    emit endResetModel();
    return;
  }
  Q_ASSERT(false);
}
