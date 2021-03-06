
#pragma once

class UserBrokerOrdersWidget : public QWidget, public Entity::Listener
{
  Q_OBJECT

public:
  UserBrokerOrdersWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager, DataService& dataService);
  ~UserBrokerOrdersWidget();

  void saveState(QSettings& settings);

public slots:
  void refresh();

private slots:
  void newBuyOrder();
  void newSellOrder();
  void submitOrder();
  void cancelOrder();
  void editedOrderPrice(const QModelIndex& index, double price);
  void editedOrderAmount(const QModelIndex& index, double amount);
  void orderSelectionChanged();
  void orderDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

private:
  QTabFramework& tabFramework;
  Entity::Manager& entityManager;
  DataService& dataService;

  UserBrokerOrdersModel ordersModel;

  QTreeView* orderView;
  QSortFilterProxyModel* proxyModel;

  QSet<EUserBrokerOrder*> selection;

  QAction* refreshAction;
  QAction* buyAction;
  QAction* sellAction;
  QAction* submitAction;
  QAction* cancelAction;

  void addOrderDraft(EUserBrokerOrder::Type type);

private:
  void updateTitle(EConnection& eDataService);
  void updateToolBarButtons();

private: // Entity::Listener
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);
};
