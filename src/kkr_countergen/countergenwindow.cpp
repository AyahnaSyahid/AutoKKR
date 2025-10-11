#include "countergenwindow.h"
#include "ui/ui_countergenwindow.h"
#include "countergenerator.h"
#include "requestparser.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>

CounterGenWindow::CounterGenWindow(QWidget *parent)
  : ui(new Ui::CounterGenWindow), gen(&generateToken), generated(), QMainWindow(parent) {
  ui->setupUi(this);
}

CounterGenWindow::~CounterGenWindow() {
  delete ui;
}

void CounterGenWindow::setGenerator(GeneratorPtr G){
  gen = G;
}

void CounterGenWindow::on_buatButton_clicked() {
  if(!ui->strDisplay->toPlainText().isEmpty()) {
    QMessageBox tanya(QMessageBox::Question, "Konfirmasi", "Token saat ini akan dihapus ?", QMessageBox::Yes | QMessageBox::No);
    auto yes = tanya.button(QMessageBox::Yes);
    auto no = tanya.button(QMessageBox::No);
    yes->setText("Ya");
    no->setText("Batalkan");
    tanya.setMinimumWidth(400);
    if(QMessageBox::Yes != tanya.exec()) {
      return ;
    }
  }
  
  ui->strDisplay->clear();
  
  if(!gen) { 
    QMessageBox::warning(this, "Kesalahan", "Generator belum di setel");
    return ;
  }
  QString clientId = ui->clientField->text();
  int tokenValue = ui->valueField->value();
  if(clientId.isEmpty() || tokenValue < 1) {
    QMessageBox::warning(this, "Kesalahan", "Periksa ClientID dan Jumlah yang akan diisi");
    return;
  }

  generated = gen(clientId, tokenValue);

  if(generated.isEmpty()) {
    QMessageBox::warning(this, "Kesalahan", "Generator GAGAL");
    return ;
  }
  QString encoded = generated.toBase64();
  // splitter
  QStringList lines;
  int lineLength = 64, encodedSize = encoded.size();
  for(int i=0; i<encoded.size(); i += lineLength) {
    if(i + lineLength > encodedSize)
      lines << encoded.sliced(i);
    else
      lines << encoded.mid(i, lineLength);
  }
  
  ui->strDisplay->setPlainText(lines.join("\n"));
}

void CounterGenWindow::on_simpanButton_clicked() {
  if(generated.isEmpty()) {
    QMessageBox::warning(this, "Kesalahan", "Belum ada data untuk disimpan");
    return ;
  }
  QString saveAs = QFileDialog::getSaveFileName(this, tr("Save File"),
                            QDir::home().filePath(ui->clientField->text() + ".tok"),
                            tr("Token (*.bin)"));
  if(!saveAs.isEmpty()) {
    QFile w(saveAs);
    if(w.open(QIODevice::WriteOnly)) {
      if(generated.size() != w.write(generated)) {
        QMessageBox::warning(this, "Kesalahan", "Jumlah data yang disimpan tidak sesuai\n(data corrupt)");        
      } else {
        QMessageBox::information(this, "Berhasil", QString("Disimpan di:\n%1").arg(saveAs));
        ui->strDisplay->clear();
      }
    }
    w.close();
  }
}

void CounterGenWindow::on_loadButton_clicked() {
  RequestParser rp(this);
  if(QDialog::Accepted == rp.exec()) {
    QString dec = rp.property("request").toString();
    QStringList splitted = dec.split("::");
    if(splitted.count() < 3) {
      QMessageBox::warning(this, "Kesalahan", "Tidak Valid");
      return ;
    }
    ui->clientField->setText(splitted.at(0));
    ui->valueField->setValue(splitted.at(2).toInt());
    ui->strDisplay->clear();
  }
}
