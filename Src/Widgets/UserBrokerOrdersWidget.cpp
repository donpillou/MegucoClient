
#include "stdafx.h"

UserBrokerOrdersWidget::UserBrokerOrdersWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager, DataService& dataService) :
  QWidget(&tabFramework), tabFramework(tabFramework), entityManager(entityManager), dataService(dataService), ordersModel(entityManager)
{
  entityManager.registerListener<EConnection>(*this);

  setWindowTitle(tr("Orders"));

  QToolBar* toolBar = new QToolBar(this);
  toolBar->setStyleSheet("QToolBar { border: 0px }");
  toolBar->setIconSize(QSize(16, 16));
  toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  refreshAction = toolBar->addAction(QIcon(":/Icons/arrow_refresh.png"), tr("&Refresh"));
  refreshAction->setEnabled(false);
  refreshAction->setShortcut(QKeySequence(QKeySequence::Refresh));
  connect(refreshAction, SIGNAL(triggered()), this, SLOT(refresh()));
  
  buyAction = toolBar->addAction(QIcon(":/Icons/bitcoin_add.png"), tr("&Buy"));
  buyAction->setEnabled(false);
  buyAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_B));
  connect(buyAction, SIGNAL(triggered()), this, SLOT(newBuyOrder()));
  sellAction = toolBar->addAction(QIcon(":/Icons/money_add.png"), tr("&Sell"));
  sellAction->setEnabled(false);
  sellAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_S));
  connect(sellAction, SIGNAL(triggered()), this, SLOT(newSellOrder()));

  submitAction = toolBar->addAction(QIcon(":/Icons/bullet_go.png"), tr("S&ubmit"));
  submitAction->setEnabled(false);
  submitAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_U));
  connect(submitAction, SIGNAL(triggered()), this, SLOT(submitOrder()));

  cancelAction = toolBar->addAction(QIcon(":/Icons/cancel2.png"), tr("&Cancel"));
  cancelAction->setEnabled(false);
  cancelAction->setShortcut(QKeySequence(Qt::Key_Delete));
  connect(cancelAction, SIGNAL(triggered()), this, SLOT(cancelOrder()));

  orderView = new QTreeView(this);
  orderView->setUniformRowHeights(true);
  proxyModel = new UserBrokerOrdersSortProxyModel(this);
  proxyModel->setDynamicSortFilter(true);
  proxyModel->setSourceModel(&ordersModel);
  orderView->setModel(proxyModel);
  orderView->setSortingEnabled(true);
  orderView->setRootIsDecorated(false);
  orderView->setAlternatingRowColors(true);

  class OrderDelegate : public QStyledItemDelegate
  {
  public:
    OrderDelegate(QObject* parent) : QStyledItemDelegate(parent) {}
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
      QWidget* widget = QStyledItemDelegate::createEditor(parent, option, index);
      QDoubleSpinBox* spinBox = qobject_cast<QDoubleSpinBox*>(widget);
      if(spinBox)
        spinBox->setDecimals(8);
      return widget;
    }
  };
  orderView->setItemDelegateForColumn((int)UserBrokerOrdersModel::Column::amount, new OrderDelegate(this));
  orderView->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::DoubleClicked);
  orderView->setSelectionMode(QAbstractItemView::ExtendedSelection);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(toolBar);
  layout->addWidget(orderView);
  setLayout(layout);

  connect(&ordersModel, SIGNAL(editedOrderPrice(const QModelIndex&, double)), this, SLOT(editedOrderPrice(const QModelIndex&, double)));
  connect(&ordersModel, SIGNAL(editedOrderAmount(const QModelIndex&, double)), this, SLOT(editedOrderAmount(const QModelIndex&, double)));
  connect(orderView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(orderSelectionChanged()));
  connect(&ordersModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(orderDataChanged(const QModelIndex&, const QModelIndex&)));

  QHeaderView* headerView = orderView->header();
  headerView->resizeSection(0, 50);
  headerView->resizeSection(1, 60);
  headerView->resizeSection(2, 110);
  headerView->resizeSection(3, 85);
  headerView->resizeSection(4, 100);
  headerView->resizeSection(5, 85);
  headerView->resizeSection(6, 85);
  orderView->sortByColumn(2);
  settings.beginGroup("Orders");
  headerView->restoreState(settings.value("HeaderState").toByteArray());
  settings.endGroup();
  headerView->setStretchLastSection(false);
  headerView->setResizeMode(0, QHeaderView::Stretch);
}

UserBrokerOrdersWidget::~UserBrokerOrdersWidget()
{
  entityManager.unregisterListener<EConnection>(*this);
}

void UserBrokerOrdersWidget::saveState(QSettings& settings)
{
  settings.beginGroup("Orders");
  settings.setValue("HeaderState", orderView->header()->saveState());
  settings.endGroup();
}

void UserBrokerOrdersWidget::newBuyOrder()
{
  addOrderDraft(EUserBrokerOrder::Type::buy);
}

void UserBrokerOrdersWidget::newSellOrder()
{
  addOrderDraft(EUserBrokerOrder::Type::sell);
}

void UserBrokerOrdersWidget::addOrderDraft(EUserBrokerOrder::Type type)
{
  EConnection* eDataService = entityManager.getEntity<EConnection>(0);
  EUserBroker* eUserBroker = entityManager.getEntity<EUserBroker>(eDataService->getSelectedBrokerId());
  if(!eUserBroker)
    return;
  EBrokerType* eBrokerType = entityManager.getEntity<EBrokerType>(eUserBroker->getBrokerTypeId());
  if(!eBrokerType)
    return;
  const QString& marketName = eBrokerType->getName();
  Entity::Manager* channelEntityManager = dataService.getSubscription(marketName);
  double price = 0;
  if(channelEntityManager)
  {
    EMarketTickerData* eDataTickerData = channelEntityManager->getEntity<EMarketTickerData>(0);
    if(eDataTickerData)
      price = type == EUserBrokerOrder::Type::buy ? (eDataTickerData->getBid() + 0.01) : (eDataTickerData->getAsk() - 0.01);
  }

  EUserBrokerOrderDraft& eBotMarketOrderDraft = dataService.createBrokerOrderDraft(type, price);
  QModelIndex amountProxyIndex = ordersModel.getDraftAmountIndex(eBotMarketOrderDraft);
  QModelIndex amountIndex = proxyModel->mapFromSource(amountProxyIndex);
  orderView->setCurrentIndex(amountIndex);
  orderView->edit(amountIndex);
}

void UserBrokerOrdersWidget::submitOrder()
{
  QModelIndexList selection = orderView->selectionModel()->selectedRows();
  foreach(const QModelIndex& proxyIndex, selection)
  {
    QModelIndex index = proxyModel->mapToSource(proxyIndex);
    EUserBrokerOrder* eBotMarketOrder = (EUserBrokerOrder*)index.internalPointer();
    if(eBotMarketOrder->getState() == EUserBrokerOrder::State::draft)
      dataService.submitBrokerOrderDraft(*(EUserBrokerOrderDraft*)eBotMarketOrder);
  }
}

void UserBrokerOrdersWidget::cancelOrder()
{
  QModelIndexList selection = orderView->selectionModel()->selectedRows();
  QList<EUserBrokerOrder*> ordersToRemove;
  QList<EUserBrokerOrderDraft*> draftsToRemove;
  QList<EUserBrokerOrder*> ordersToCancel;
  foreach(const QModelIndex& proxyIndex, selection)
  {
    QModelIndex index = proxyModel->mapToSource(proxyIndex);
    EUserBrokerOrder* eBotMarketOrder = (EUserBrokerOrder*)index.internalPointer();
    switch(eBotMarketOrder->getState())
    {
    case EUserBrokerOrder::State::draft:
      draftsToRemove.append((EUserBrokerOrderDraft*)eBotMarketOrder);
      break;
    case EUserBrokerOrder::State::canceled:
    case EUserBrokerOrder::State::closed:
      ordersToRemove.append(eBotMarketOrder);
      break;
    case EUserBrokerOrder::State::open:
      ordersToCancel.append(eBotMarketOrder);
      break;
    default:
      break;
    }
  }
  while(!ordersToCancel.isEmpty())
  {
    QList<EUserBrokerOrder*>::Iterator last = --ordersToCancel.end();
    dataService.cancelBrokerOrder(**last);
    ordersToCancel.erase(last);
  }
  while(!draftsToRemove.isEmpty())
  {
    QList<EUserBrokerOrderDraft*>::Iterator last = --draftsToRemove.end();
    dataService.removeBrokerOrderDraft(**last);
    draftsToRemove.erase(last);
  }
  while(!ordersToRemove.isEmpty())
  {
    QList<EUserBrokerOrder*>::Iterator last = --ordersToRemove.end();
    dataService.removeBrokerOrder(**last);
    ordersToRemove.erase(last);
  }
}

void UserBrokerOrdersWidget::editedOrderPrice(const QModelIndex& index, double price)
{
  EUserBrokerOrder* eBotMarketOrder = (EUserBrokerOrder*)index.internalPointer();
  dataService.updateBrokerOrder(*eBotMarketOrder, price, eBotMarketOrder->getAmount());
}

void UserBrokerOrdersWidget::editedOrderAmount(const QModelIndex& index, double amount)
{
  EUserBrokerOrder* eBotMarketOrder = (EUserBrokerOrder*)index.internalPointer();
  dataService.updateBrokerOrder(*eBotMarketOrder, eBotMarketOrder->getPrice(), amount);
}

void UserBrokerOrdersWidget::orderSelectionChanged()
{
  QModelIndexList modelSelection = orderView->selectionModel()->selectedRows();
  selection.clear();
  for(QModelIndexList::Iterator i = modelSelection.begin(), end = modelSelection.end(); i != end; ++i)
  {
    QModelIndex modelIndex = proxyModel->mapToSource(*i);
    EUserBrokerOrder* eOrder = (EUserBrokerOrder*)modelIndex.internalPointer();
    selection.insert(eOrder);
  }
  updateToolBarButtons();
}

void UserBrokerOrdersWidget::orderDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
  QModelIndex index = topLeft;
  for(int i = topLeft.row(), end = bottomRight.row();;)
  {
    EUserBrokerOrder* eOrder = (EUserBrokerOrder*)index.internalPointer();
    if(selection.contains(eOrder))
    {
      orderSelectionChanged();
      break;
    }
    if(i++ == end)
      break;
    index = index.sibling(i, 0);
  }
}

void UserBrokerOrdersWidget::updateToolBarButtons()
{
  EConnection* eDataService = entityManager.getEntity<EConnection>(0);
  bool connected = eDataService->getState() == EConnection::State::connected;
  bool brokerSelected = connected && eDataService->getSelectedBrokerId() != 0;
  bool canCancel = connected && !selection.isEmpty();

  bool draftSelected = false;
  for(QSet<EUserBrokerOrder*>::Iterator i = selection.begin(), end = selection.end(); i != end; ++i)
  {
    EUserBrokerOrder* eBotMarketOrder = *i;
    if(eBotMarketOrder->getState() == EUserBrokerOrder::State::draft)
    {
      draftSelected = true;
      break;
    }
  }

  refreshAction->setEnabled(brokerSelected);
  buyAction->setEnabled(brokerSelected);
  sellAction->setEnabled(brokerSelected);
  submitAction->setEnabled(brokerSelected && draftSelected);
  cancelAction->setEnabled(canCancel);
}

void UserBrokerOrdersWidget::refresh()
{
  dataService.refreshBrokerOrders();
  dataService.refreshBrokerBalance();
}

void UserBrokerOrdersWidget::updateTitle(EConnection& eDataService)
{
  QString stateStr = eDataService.getStateName();
  QString title;
  if(stateStr.isEmpty())
    stateStr = eDataService.getMarketOrdersState();
  if(stateStr.isEmpty())
    title = tr("Orders");
  else
    title = tr("Orders (%2)").arg(stateStr);

  setWindowTitle(title);
  tabFramework.toggleViewAction(this)->setText(tr("Orders"));
}

void UserBrokerOrdersWidget::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
{
  switch ((EType)newEntity.getType())
  {
  case EType::connection:
    updateTitle(*dynamic_cast<EConnection*>(&newEntity));
    updateToolBarButtons();
    break;
  default:
    break;
  }
}
