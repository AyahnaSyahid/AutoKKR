#include "tokenrefilldialog.h"
#include "ui/ui_tokenrefilldialog.h"
#include "polledsettings.h"
#include "kkr_countergen/countergenerator.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QDateTime>
#include <QDir>

class IndoMessageBox;

TokenRefillDialog::TokenRefillDialog(PolledSettings *ps, QWidget *parent)
  : m_ps(ps), ui(new Ui::TokenRefillDialog), QDialog(parent) {
  ui->setupUi(this);
}

TokenRefillDialog::~TokenRefillDialog() {
  delete ui;
}

void TokenRefillDialog::on_okButton_clicked() {
  if (ui->plainTextEdit->toPlainText().isEmpty()) {
    QMessageBox::warning(this, "Kesalahan", "Token belum diisi");
    return;
  }
  
  bool verified;
  QByteArray asByte = QByteArray::fromBase64(ui->plainTextEdit->toPlainText().toUtf8());
  auto token = validateToken(asByte, &verified);
  if (!verified) {
    // message "Invalid Token"
    QMessageBox::warning(this, "Kesalahan", "Kode Token tidak valid\nValidasi input gagal");
    return;
  }
  
  // Extract and validate payload (defense-in-depth)
  auto a = token.split("::");
  if (a.size() != 3) {
    // message "Invalid Token Format"
    QMessageBox::warning(this, "Kesalahan", "Kode Token tidak valid\nPastikan anda menginputnya dengan benar!");
    qDebug() << "Token split failed:" << a;
    return;
  }
  
  if (a.at(0) != m_ps->installID()) {
    QMessageBox::warning(this, "Kesalahan", "ID pemasangan token tidak sesuai");
    return;
  }
  
  qDebug() << "Raw LastRefillTime:" << m_ps->appData().LastRefillTime;
  
  QDateTime last = QDateTime::fromString(m_ps->appData().LastRefillTime, "yyyy-MM-dd HH:mm:ss");
  QDateTime next = QDateTime::fromString(a.at(1), "yyyy-MM-dd HH:mm:ss");
  
  // Always require next valid
  if (!next.isValid()) {
    QMessageBox::warning(this, "Kesalahan", "Tanggal dalam Kode Token tidak valid");
    qDebug() << "Token date invalid:" << next << "Raw:" << a.at(1);
    return;
  }
  
  // First-use grace: If last invalid, proceed (no prior refill)
  if (last.isValid()) {
    // Check for reuse only if last exists
    if (last >= next) {
      QMessageBox::warning(this, "Penggunaan Ulang", "Maaf,\nKode token ini telah digunakan sebelumnya");
      qDebug() << "Reuse detected: last" << last << ">= next" << next;
      return;
    }
  } else {
    qDebug() << "First use: No prior last refill";
  }
  
  // Optional: Check value is positive/non-zero
  bool valueOk;
  int value = a.at(2).toInt(&valueOk);
  if (!valueOk || value <= 0) {
    // message "Invalid Token Value"
    QMessageBox::warning(this, "Kesalahan", "Nilai didalam kode token tidak dapat diparse (korupt)");
    qDebug() << "Value parse failed:" << a.at(2);
    return;
  }
  
  m_ps->refillCounter(value);
  m_ps->appData().LastRefillTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
  accept();
}

void TokenRefillDialog::on_fileButton_clicked() {
  QString file = QFileDialog::getOpenFileName(this, "Pilih token file", QDir::homePath(), "Token File (*.bin)");
  if(file.isEmpty()) {
    return ;
  }
  QFileInfo info(file);
  QFile opf(file);
  if(info.size() == 0 || info.size() > 2000000) {
    QMessageBox::warning(this, "Kesalahan", "Proses dibatalkan karena file yang anda pilih terlalu besar");
    return ;
  }
  if(!opf.open(QIODevice::ReadOnly)) {
    QMessageBox::warning(this, "Kesalahan", "Tidak dapat membaca file");
    return ;
  }
  QByteArray data = opf.readAll();
  opf.close();
  ui->plainTextEdit->setPlainText(data.toBase64());
}