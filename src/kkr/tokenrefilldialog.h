#ifndef TOKENREFILLDIALOG_H
#define TOKENREFILLDIALOG_H

#include <QDialog>

namespace Ui {
  class TokenRefillDialog;
}

class PolledSettings;

class TokenRefillDialog : public QDialog {
  Q_OBJECT

public:
  explicit TokenRefillDialog(PolledSettings *ps, QWidget *parent=nullptr);
  ~TokenRefillDialog();

private slots:
  void on_okButton_clicked();
  void on_fileButton_clicked();

private:
  PolledSettings *m_ps;
  Ui::TokenRefillDialog *ui;
};


#endif // TOKENREFILLDIALOG_H