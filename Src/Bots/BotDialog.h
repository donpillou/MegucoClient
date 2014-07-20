
#pragma once

class BotDialog : public QDialog
{
  Q_OBJECT

public:
  BotDialog(QWidget* parent, Entity::Manager& entityManager);

  QString getName() const {return nameEdit->text();}
  quint32 getEngineId() const;
  quint32 getMarketId() const;
  //double getBalanceBase() const {return balanceBaseSpinBox->value();}
  //double getBalanceComm() const {return balanceCommSpinBox->value();}

private:
  Entity::Manager& entityManager;
  QLineEdit* nameEdit;
  QComboBox* engineComboBox;
  QComboBox* marketComboBox;
  //QDoubleSpinBox * balanceBaseSpinBox;
  //QDoubleSpinBox * balanceCommSpinBox;
  QPushButton* okButton;
  //QLabel* balanceBaseLabel;
  //QLabel* balanceCommLabel;

private:
  virtual void showEvent(QShowEvent* event);

private slots:
  void textChanged();
  //void marketSelectionChanged(int index);
};
