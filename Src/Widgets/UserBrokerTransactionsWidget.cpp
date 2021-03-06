
#include "stdafx.h"

UserBrokerTransactionsWidget::UserBrokerTransactionsWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager, DataService& dataService) :
  QWidget(&tabFramework), tabFramework(tabFramework), entityManager(entityManager), dataService(dataService), transactionsModel(entityManager)
{
  entityManager.registerListener<EConnection>(*this);

  setWindowTitle(tr("Transactions"));

  QToolBar* toolBar = new QToolBar(this);
  toolBar->setStyleSheet("QToolBar { border: 0px }");
  toolBar->setIconSize(QSize(16, 16));
  toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  refreshAction = toolBar->addAction(QIcon(":/Icons/arrow_refresh.png"), tr("&Refresh"));
  refreshAction->setEnabled(false);
  refreshAction->setShortcut(QKeySequence(QKeySequence::Refresh));
  connect(refreshAction, SIGNAL(triggered()), this, SLOT(refresh()));

  transactionView = new QTreeView(this);
  transactionView->setUniformRowHeights(true);
  proxyModel = new UserBrokerTransactionsSortProxyModel(this);
  proxyModel->setSourceModel(&transactionsModel);
  proxyModel->setDynamicSortFilter(true);
  transactionView->setModel(proxyModel);
  transactionView->setSortingEnabled(true);
  transactionView->setRootIsDecorated(false);
  transactionView->setAlternatingRowColors(true);
  //transactionView->setSelectionMode(QAbstractItemView::ExtendedSelection);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(toolBar);
  layout->addWidget(transactionView);
  setLayout(layout);

  QHeaderView* headerView = transactionView->header();
  headerView->resizeSection(0, 50);
  headerView->resizeSection(1, 110);
  headerView->resizeSection(2, 85);
  headerView->resizeSection(3, 100);
  headerView->resizeSection(4, 85);
  headerView->resizeSection(5, 75);
  headerView->resizeSection(6, 85);
  transactionView->sortByColumn(1);
  settings.beginGroup("Transactions");
  headerView->restoreState(settings.value("HeaderState").toByteArray());
  settings.endGroup();
  headerView->setStretchLastSection(false);
  headerView->setResizeMode(0, QHeaderView::Stretch);
}

UserBrokerTransactionsWidget::~UserBrokerTransactionsWidget()
{
  entityManager.unregisterListener<EConnection>(*this);
}

void UserBrokerTransactionsWidget::saveState(QSettings& settings)
{
  settings.beginGroup("Transactions");
  settings.setValue("HeaderState", transactionView->header()->saveState());
  settings.endGroup();
}

void UserBrokerTransactionsWidget::updateToolBarButtons()
{
  EConnection* eDataService = entityManager.getEntity<EConnection>(0);
  bool connected = eDataService->getState() == EConnection::State::connected;
  bool brokerSelected = connected && eDataService->getSelectedBrokerId() != 0;

  refreshAction->setEnabled(brokerSelected);
}

void UserBrokerTransactionsWidget::refresh()
{
  dataService.refreshBrokerTransactions();
}

void UserBrokerTransactionsWidget::updateTitle(EConnection& eDataService)
{
  QString stateStr = eDataService.getStateName();
  QString title;
  if(stateStr.isEmpty())
    stateStr = eDataService.getMarketTransitionsState();
  if(stateStr.isEmpty())
    title = tr("Transactions");
  else
    title = tr("Transactions (%2)").arg(stateStr);

  setWindowTitle(title);
  tabFramework.toggleViewAction(this)->setText(tr("Transactions"));
}

void UserBrokerTransactionsWidget::updatedEntitiy(Entity& oldEntity, Entity& newEntity)
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
