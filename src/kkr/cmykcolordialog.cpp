#include "cmykcolordialog.h"
#include "ui/ui_cmykcolordialog.h"
#include "colorconverter.h"
#include <QPalette>
#include <QDir>
#include <QInputDialog>
#include <QColorDialog>
#include <QColor>
#include <QMenu>
#include <QSettings>
#include <QStandardItem>
#include <QItemSelectionModel>
#include <QtMath>
#include <QtDebug>

CMYKColorDialog::CMYKColorDialog(const QColor& cl, QWidget* parent)
: ui(new Ui::CMYKColorDialog), model(new QStandardItemModel(0, 2, this)), QDialog(parent) {
  ui->setupUi(this);
  setSpinValues(cl);
  QSettings settings(QDir::home().filePath(".kkr_color"), QSettings::IniFormat);
  settings.beginGroup("colors");
  for(auto key : settings.allKeys()) {
    QColor color = settings.value(key).value<QColor>();
    QColor proof = ColorManagement::proof_of(color);
    auto nameItem = new QStandardItem(key);
    auto colorItem = new QStandardItem(color.name());
    colorItem->setBackground(proof);
    colorItem->setData(color);
    colorItem->setTextAlignment(Qt::AlignCenter);
    colorItem->setEditable(false);
    if(qGray(proof.rgb()) > 128) {
      colorItem->setForeground(Qt::white);
    }
    model->appendRow({nameItem, colorItem});
  }
  settings.endGroup();
  ui->tableView->setModel(model);
  ui->tableView->installEventFilter(this);
  model->setHorizontalHeaderLabels({"Nama", "Kode RGB"});
  connect(model, &QStandardItemModel::rowsInserted, this, &CMYKColorDialog::on_rowsInserted);
  connect(ui->tableView, &QAbstractItemView::clicked, this, &CMYKColorDialog::on_clickedIndex);
  ui->label_7->setText(QString("Warna tersimpan: %1").arg(model->rowCount()));
}

CMYKColorDialog::~CMYKColorDialog() {
  delete ui;
}

void CMYKColorDialog::anySpinBoxChanged() {
  int c = ui->cyanBox->value() * 255 / 100,
      m = ui->magentaBox->value() * 255 / 100,
      y = ui->yellowBox->value() * 255 / 100,
      k = ui->blackBox->value() * 255 / 100,
      a = ui->alphaBox->value() * 255 / 100;
  
  auto cmyk = QColor::fromCmyk(c, m, y, k, a);
  auto proof = ColorManagement::proof_of(cmyk);
  proof.setAlpha(a);
  auto palette = ui->label->palette();
  palette.setColor(QPalette::Window, proof);
  ui->label->setPalette(palette);
}

void CMYKColorDialog::setColor(const QColor& clr) {
  
}

void CMYKColorDialog::on_terapkanButton_clicked() {
  emit currentColorChanged(getColor());
  accept();
}

QColor CMYKColorDialog::getColor() const {
  int c = ui->cyanBox->value() * 255 / 100,
      m = ui->magentaBox->value() * 255 / 100,
      y = ui->yellowBox->value() * 255 / 100,
      k = ui->blackBox->value() * 255 / 100,
      a = ui->alphaBox->value() * 255 / 100;
  return QColor::fromCmyk(c, m, y, k, a);
}

void CMYKColorDialog::on_addButton_clicked() {
  QString name = QInputDialog::getText(this, "Tetapkan Nama Template Warna", "Template");
  if(name.isEmpty()) return;
  auto color = getColor();
  auto colorItem = new QStandardItem(color.name());
  colorItem->setText(color.name());
  colorItem->setData(color);
  colorItem->setData(ColorManagement::proof_of(color), Qt::BackgroundRole);
  colorItem->setEnabled(false);
  colorItem->setTextAlignment(Qt::AlignCenter);
  model->appendRow({new QStandardItem(name), colorItem});
  sync();
}

void CMYKColorDialog::setLabelColor(const QColor& cl) {
}

void CMYKColorDialog::setSpinValues(const QColor& c) {
  ui->cyanBox->setValue(qRound(c.cyan() * 100 / 255.0));
  ui->magentaBox->setValue(qRound(c.magenta() * 100 / 255.0));
  ui->yellowBox->setValue(qRound(c.yellow() * 100 / 255.0));
  ui->blackBox->setValue(qRound(c.black() * 100 / 255.0));
  ui->alphaBox->setValue(qRound(c.alpha() * 100 / 255.0));
}

void CMYKColorDialog::on_rowsInserted(const QModelIndex& mi, int fl, int ls) {
  ui->label_7->setText(QString("Warna tersimpan: %1").arg(model->rowCount()));
}

void CMYKColorDialog::sync() {
  QSettings settings(QDir::home().filePath(".kkr_color"), QSettings::IniFormat);
  settings.remove("colors");
  settings.beginGroup("colors");
  for(int row=0; row<model->rowCount(); ++row) {
    QString key = model->itemFromIndex(model->index(row, 0))->text();
    QColor colorData = model->itemFromIndex(model->index(row, 1))->data().value<QColor>();
    settings.setValue(key, colorData);
  }
  settings.endGroup();
  settings.sync();
}

void CMYKColorDialog::on_clickedIndex(const QModelIndex& mi) {
  auto item = model->itemFromIndex(mi.siblingAtColumn(1));
  QColor userColor = item->data().value<QColor>();
  if(!userColor.isValid()) return;
  ui->cyanBox->setValue(userColor.cyan() * 100 / 255);
  ui->magentaBox->setValue(userColor.magenta() * 100 / 255);
  ui->yellowBox->setValue(userColor.yellow() * 100 / 255);
  ui->blackBox->setValue(userColor.black() * 100 / 255);
  ui->alphaBox->setValue(userColor.alpha() * 100 / 255);
}

void CMYKColorDialog::on_tableView_doubleClicked(const QModelIndex& mi) {
  Q_UNUSED(mi);
  emit currentColorChanged(getColor());
  accept();
}

bool CMYKColorDialog::eventFilter(QObject* watched, QEvent *ev) {
  if(watched == ui->tableView) {
    if(ev->type() == QEvent::Leave) {
      ui->tableView->setCurrentIndex(QModelIndex());
      ev->accept();
      return true;
    }
  }
  return QDialog::eventFilter(watched, ev);
}

void CMYKColorDialog::on_tableView_customContextMenuRequested(const QPoint& p) {
  if(ui->tableView->indexAt(p).isValid()) {
    QMenu context;
    auto act = context.addAction("Hapus");
    if(act == context.exec(ui->tableView->viewport()->mapToGlobal(p))) {
      QModelIndex mi= ui->tableView->indexAt(p);
      model->removeRow(mi.row());
      sync();
    }
  }
}