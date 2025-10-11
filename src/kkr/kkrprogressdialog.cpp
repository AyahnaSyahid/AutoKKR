#include "kkrprogressdialog.h"
#include "ui/ui_kkrprogressdialog.h"

#include <QMessageBox>
#include <QThreadPool>

KKRProgressDialog::KKRProgressDialog(QWidget* parent)
: ui(new Ui::KKRProgressDialog), QDialog(parent)
{
  ui->setupUi(this);
}

KKRProgressDialog::~KKRProgressDialog() {
  delete ui;
}

void KKRProgressDialog::setMessage(const QString& m) {
  ui->msgLabel->setText(m);
}
void KKRProgressDialog::setMinimum(int c) {
  ui->progressBar->setMinimum(c);
}
void KKRProgressDialog::setMaximum(int c) {
  ui->progressBar->setMaximum(c);
}
void KKRProgressDialog::setCurrent(int c) {
  ui->progressBar->setValue(c);
}
void KKRProgressDialog::onStopped() {
  // Runnable stopped
  accept();
}
void KKRProgressDialog::onStarted() {
  // Runnable started
}
void KKRProgressDialog::onError(const QString& m) {
  QMessageBox::critical(this, "Error", m);
  reject();
}

void KKRProgressDialog::onCanceled() {
  // Progress on runnable canceled
  reject();
}

void KKRProgressDialog::on_cancelButton_clicked() {
  emit requestCancel();
}