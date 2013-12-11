
#pragma once

class LogWidget : public QWidget
{
  Q_OBJECT

public:
  LogWidget(QWidget* parent, QSettings& settings, LogModel& logModel);

  void saveState(QSettings& settings);

private slots:
  void setMarket(Market* market);

private:
  LogModel& logModel;
  QTreeView* logView;
  QSortFilterProxyModel* proxyModel;
};