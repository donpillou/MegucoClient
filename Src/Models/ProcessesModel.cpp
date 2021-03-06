
#include "stdafx.h"

ProcessesModel::ProcessesModel(Entity::Manager& entityManager) :
  entityManager(entityManager)
{
  entityManager.registerListener<EProcess>(*this);
}

ProcessesModel::~ProcessesModel()
{
  entityManager.unregisterListener<EProcess>(*this);
}

QModelIndex ProcessesModel::index(int row, int column, const QModelIndex& parent) const
{
  if(hasIndex(row, column, parent))
    return createIndex(row, column, processes.at(row));
  return QModelIndex();
}

QModelIndex ProcessesModel::parent(const QModelIndex& child) const
{
  return QModelIndex();
}

int ProcessesModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : processes.size();
}

int ProcessesModel::columnCount(const QModelIndex& parent) const
{
  return (int)Column::last + 1;
}

QVariant ProcessesModel::data(const QModelIndex& index, int role) const
{
  const EProcess* eProcess= (const EProcess*)index.internalPointer();
  if(!eProcess)
    return QVariant();

  switch(role)
  {
  case Qt::TextAlignmentRole:
    return (int)Qt::AlignLeft | (int)Qt::AlignVCenter;
  case Qt::DisplayRole:
    return eProcess->getCmd();
  }
  return QVariant();
}

QVariant ProcessesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(orientation != Qt::Horizontal)
    return QVariant();
  switch(role)
  {
  case Qt::TextAlignmentRole:
    return Qt::AlignLeft;
  case Qt::DisplayRole:
    return tr("Command");
  }
  return QVariant();
}

void ProcessesModel::addedEntity(Entity& entity)
{
  switch((EType)entity.getType())
  {
  case EType::process:
    {
      EProcess* eProcess = dynamic_cast<EProcess*>(&entity);
      int index = processes.size();
      beginInsertRows(QModelIndex(), index, index);
      processes.append(eProcess);
      endInsertRows();
      break;
    }
  default:
    Q_ASSERT(false);
    break;
  }
}

void ProcessesModel::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  switch((EType)oldEntity.getType())
  {
  case EType::userBrokerTransaction:
    {
      EProcess* oldEProcess = dynamic_cast<EProcess*>(&oldEntity);
      EProcess* newEProcess = dynamic_cast<EProcess*>(&newEntity);
      int index = processes.indexOf(oldEProcess);
      processes[index] = newEProcess; 
      QModelIndex leftModelIndex = createIndex(index, (int)Column::first, newEProcess);
      QModelIndex rightModelIndex = createIndex(index, (int)Column::last, newEProcess);
      emit dataChanged(leftModelIndex, rightModelIndex);
      break;
    }
  default:
    Q_ASSERT(false);
    break;
  }
}

void ProcessesModel::removedEntity(Entity& entity)
{
  switch((EType)entity.getType())
  {
  case EType::process:
    {
      EProcess* eProcess = dynamic_cast<EProcess*>(&entity);
      int index = processes.indexOf(eProcess);
      beginRemoveRows(QModelIndex(), index, index);
      processes.removeAt(index);
      endRemoveRows();
      break;
    }
  default:
    Q_ASSERT(false);
    break;
  }
}

void ProcessesModel::removedAll(quint32 type)
{
  switch((EType)type)
  {
  case EType::process:
    if(!processes.isEmpty())
    {
      emit beginResetModel();
      processes.clear();
      emit endResetModel();
    }
    break;
  default:
    Q_ASSERT(false);
    break;
  }
}
