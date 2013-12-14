
#include "stdafx.h"

LogWidget::LogWidget(QWidget* parent, QSettings& settings, LogModel& logModel) : QWidget(parent), logModel(logModel)//, autoScrollEnabled(true)
{
  logView = new QTreeView(this);
  proxyModel = new QSortFilterProxyModel(this);
  proxyModel->setDynamicSortFilter(true);
  logView->setModel(proxyModel);
  logView->setSortingEnabled(true);
  logView->setRootIsDecorated(false);
  logView->setAlternatingRowColors(true);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(logView);
  setLayout(layout);

  proxyModel->setSourceModel(&logModel);
  connect(proxyModel, SIGNAL(rowsAboutToBeInserted(const QModelIndex&, int, int)), this, SLOT(checkAutoScroll(const QModelIndex&, int, int)));
  QHeaderView* headerView = logView->header();
  headerView->resizeSection(0, 22);
  headerView->resizeSection(1, 110);
  headerView->resizeSection(2, 200);
  logView->sortByColumn(1, Qt::AscendingOrder);
  headerView->restoreState(settings.value("LogHeaderState").toByteArray());
}

void LogWidget::saveState(QSettings& settings)
{
  settings.setValue("LogHeaderState", logView->header()->saveState());
}

void LogWidget::setMarket(Market* market)
{
}

void LogWidget::checkAutoScroll(const QModelIndex& index, int, int)
{
  QScrollBar* scrollBar = logView->verticalScrollBar();
  if(scrollBar->value() == scrollBar->maximum())
    QTimer::singleShot(1, this, SLOT(autoScroll()));
}

void LogWidget::autoScroll()
{
  QScrollBar* scrollBar = logView->verticalScrollBar();
  scrollBar->setValue(scrollBar->maximum());
}
