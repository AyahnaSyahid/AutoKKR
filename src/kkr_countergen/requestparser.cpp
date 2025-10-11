#include "requestparser.h"
#include "ui/ui_requestparser.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QMessageBox>

// public:
RequestParser::RequestParser(QWidget *parent) 
: ui(new Ui::RequestParserDialog), QDialog(parent) {
  ui->setupUi(this);
  setProperty("invalidCode", true);
}
RequestParser::~RequestParser() {
  delete ui;
}

// private slots:
void RequestParser::on_loadButton_clicked() {
  QString requestFile = QFileDialog::getOpenFileName(this, "Pilih file request", "", "Request File (*.req.bin) ;; All Files (*.*)");
  if(requestFile.isEmpty()) return;
  QFileInfo info(requestFile);
  if(info.size() > 3000000) {
    QMessageBox::information(this, "Kesalahan", QString("%1 tidak terlihat seperti file request").arg(requestFile));
    return ;
  }
  QFile rf(requestFile);
  if(!rf.open(QIODevice::ReadOnly)) {
    QMessageBox::information(this, "Kesalahan", "Tidak dapat membuka file");
    return;
  }
  QByteArray base64 =  rf.readAll();
  rf.close();
  
  ui->plainTextEdit->setPlainText(base64);
}

void RequestParser::on_testButton_clicked() {
  if(ui->plainTextEdit->toPlainText().isEmpty()) {
    QMessageBox::information(this, "Kesalahan", "Belum ada data untuk diuji");
    setProperty("invalidCode", true);
    return ;
  }
  QByteArray compressed = QByteArray::fromBase64(ui->plainTextEdit->toPlainText().toUtf8(), QByteArray::Base64Encoding | QByteArray::AbortOnBase64DecodingErrors);
  QByteArray decoded = qUncompress(compressed);
  if(decoded.isEmpty()) {
    QMessageBox::warning(this, "Kesalahan", "Input tidak valid");
    setProperty("invalidCode", true);
    return ;
  }
  QMessageBox::information(this, "Ok", decoded);
  setProperty("invalidCode", false);
  setProperty("request", decoded);
}

void RequestParser::on_okButton_clicked() {
  bool invalid = property("invalidCode").toBool();
  if(!invalid) {
    accept();
  }
}
