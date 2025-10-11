#ifndef TOKENREQUESTDIALOG_H
#define TOKENREQUESTDIALOG_H

#include "polledsettings.h"

#include <QDialog>
namespace Ui {
  class TokenRequestDialog;
};

class TokenRequestDialog : public QDialog {
  Q_OBJECT
public:
  explicit TokenRequestDialog(PolledSettings* ps, QWidget *parent=nullptr);
  ~TokenRequestDialog();
  
private slots:
  void on_buatButton_clicked();
  
private:
  Ui::TokenRequestDialog *ui;
  PolledSettings *m_ps;
};

#endif // TOKENREQUESTDIALOG_H