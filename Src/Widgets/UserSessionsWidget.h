
#pragma once

class UserSessionsWidget : public QWidget, public Entity::Listener
{
  Q_OBJECT

public:
  UserSessionsWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager, DataService& dataService);
  ~UserSessionsWidget();

  void saveState(QSettings& settings);

private slots:
  void addBot();
  void optimize();
  void simulate();
  void activate();
  void cancelBot();
  void sessionSelectionChanged();
  void sessionDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);
  void sessionDataRemoved(const QModelIndex& parent, int start, int end);
  void sessionDataReset();
  void sessionDataAdded(const QModelIndex& parent, int start, int end);

private:
  QTabFramework& tabFramework;
  Entity::Manager& entityManager;
  DataService& dataService;

  UserSessionsModel sessionsModel;
  //UserSessionOrdersModel ordersModel;
  //SessionTransactionModel transactionModel;

  QTreeView* sessionView;
  QSortFilterProxyModel* proxyModel;

  QSet<EUserSession*> selection;
  quint64 selectedSessionId;

  QAction* addAction;
  QAction* optimizeAction;
  QAction* simulateAction;
  QAction* activateAction;
  QAction* cancelAction;

private:
  void updateTitle(EConnection& eDataService);
  void updateToolBarButtons();
  void updateSelection();

private: // Entity::Listener
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);
};
