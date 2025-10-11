#include "tokenrequestdialog.h"
#include "ui/ui_tokenrequestdialog.h"

TokenRequestDialog::TokenRequestDialog(PolledSettings *ps, QWidget *parent) 
  : m_ps(ps), ui(new Ui::TokenRequestDialog), QDialog(parent) {
  ui->setupUi(this);
  auto data = ps->appData();
  QString t = ui->label_3->text();
  ui->label_3->setText(t.arg(QString::number(data.CounterLeft), QString::number(data.UsageCounter)));
}

TokenRequestDialog::~TokenRequestDialog() {
  delete ui;
}

void TokenRequestDialog::on_buatButton_clicked() {
  auto mId = m_ps->installID();
  auto lus = m_ps->appData().LastRefillTime;
  auto rt = QString::number(ui->spinBox->value());
  auto ccc = QString::number(m_ps->appData().CounterLeft);
  auto G = QString("%1::%2::%3::%4").arg(mId, lus, rt, ccc);
  QByteArray ba = qCompress(G.toUtf8());
  ui->plainTextEdit->setPlainText(ba.toBase64());
}