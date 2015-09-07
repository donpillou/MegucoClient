
#pragma once

class MarketsWidget : public QWidget, public Entity::Listener
{
  Q_OBJECT

public:
  MarketsWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager, DataService& dataService);
  ~MarketsWidget();

  void saveState(QSettings& settings);

private:
  QTabFramework& tabFramework;
  Entity::Manager& entityManager;
  DataService& dataService;

  BotMarketModel botMarketModel;

  QTreeView* marketView;
  QSortFilterProxyModel* proxyModel;

  QSet<EUserBroker*> selection;
  quint64 selectedMarketId;

  QAction* addAction;
  QAction* editAction;
  QAction* removeAction;

private slots:
  void addMarket();
  void editMarket();
  void removeMarket();
  void marketSelectionChanged();
  void marketDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);
  void marketDataRemoved(const QModelIndex& parent, int start, int end);
  void marketDataReset();
  void marketDataAdded(const QModelIndex& parent, int start, int end);

private:
  void updateTitle(EBotService& eBotService);
  void updateToolBarButtons();
  void updateSelection();

private: // Entity::Listener
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);
};
