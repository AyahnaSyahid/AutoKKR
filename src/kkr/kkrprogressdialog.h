#ifndef KKRPROGRESSDIALOG_H
#define KKRPROGRESSDIALOG_H

#include <QDialog>

namespace Ui {
  class KKRProgressDialog;
}

class KKRProgressDialog : public QDialog {
  Q_OBJECT

  public:
    KKRProgressDialog(QWidget *parent=nullptr);
    ~KKRProgressDialog();

  public slots:
    void setMessage(const QString& m);
    void setCurrent(int c);
    void setMaximum(int c);
    void setMinimum(int c);
    void onError(const QString&);
    void onStopped();
    void onStarted();
    void onCanceled();
    void on_cancelButton_clicked();

  signals:
    void requestCancel();
  
  private:
    Ui::KKRProgressDialog *ui;
};

#endif // KKRPROGRESSDIALOG_H