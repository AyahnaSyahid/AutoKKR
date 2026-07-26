#include "kkrmainwindow.h"

#include <QAction>
#include <QColor>
#include <QColorDialog>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeData>
#include <QModelIndex>
#include <QMouseEvent>
#include <QPalette>
#include <QPixmapCache>
#include <QResource>
#include <QStandardItem>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QThreadPool>
#include <QValidator>

#include "aboutkkrdialog.h"
#include "cmykcolordialog.h"
#include "colorconverter.h"
#include "corelchooser.h"
#include "cv/cvhelper.hpp"
#include "kkrprogressdialog.h"
#include "runnablecoreltask.h"
#include "ui/ui_kkrmainwindow.h"

// #include <QDragEnterEvent>
namespace KKRMainWindowInternal {
class Validator : public QValidator {
 public:
  Validator(KKRMainWindow* kk, QObject* parent) : kkr(kk), QValidator(parent) {}
  QValidator::State validate(QString& s, int& pos) const override {
    if (kkr->findChild<QCheckBox*>("autoCapsLock")->isChecked())
      s = s.toUpper();
    return QValidator::Acceptable;
  }

 private:
  KKRMainWindow* kkr;
};
class Delegate : public QStyledItemDelegate {
 public:
  Delegate(KKRMainWindow* parent)
      : kkr(parent),
        val(new Validator(parent, this)),
        QStyledItemDelegate(parent) {};
  ~Delegate() {};
  QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& opt,
                        const QModelIndex& ix) const override;

 private:
  KKRMainWindow* kkr;
  QValidator* val;
};
}  // namespace KKRMainWindowInternal

KKRMainWindow::KKRMainWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::KKRMainWindow),
      smodel(new QStandardItemModel(this)),
      ps("AutoKKR") {
  ui->setupUi(this);
  QSettings s;
  fontName = s.value("AutoKK/defaultFont", "NF-Le petit cochon").toString();
  QPixmapCache::setCacheLimit(10240 * 200);
  ui->listView->setModel(smodel);
  ui->label_4->installEventFilter(this);
  ui->listView->installEventFilter(this);
  ui->listView->setAcceptDrops(true);
  connect(ui->listView->selectionModel(),
          &QItemSelectionModel::selectionChanged, this,
          &KKRMainWindow::onListViewSelectionChanged);
  auto d = new KKRMainWindowInternal::Delegate(this);
  ui->listView->setItemDelegate(d);
  Q_INIT_RESOURCE(colormanagement);  // Nama dasar file .qrc tanpa ekstensi
  Q_INIT_RESOURCE(eruces);           // Nama dasar file .qrc tanpa ekstensi
  setWindowIcon(QIcon(":/images/app-logo-hd"));
  ColorManagement::init();
  auto corelChooser = new CorelChooser(this);
  auto act = new QAction("Corel", this);
  connect(act, &QAction::triggered, corelChooser, &CorelChooser::openDialog);
  auto mbar = menuBar();
  mbar->addAction(act);
  ui->fontComboBox->setCurrentFont(QFont("NF-Le petit cochon"));
  connect(ui->fontComboBox, &QFontComboBox::currentFontChanged, this,
          &KKRMainWindow::onComboFontChanged);
  ui->fontComboBox->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ui->fontComboBox, &QFontComboBox::customContextMenuRequested, this, &KKRMainWindow::onComboFontContextMenuRequested);
}

KKRMainWindow::~KKRMainWindow() { delete ui; }

void KKRMainWindow::on_actionImportFile_triggered() {
  // import files
  auto files = QFileDialog::getOpenFileNames(this, "Pilih file untuk diproses",
                                             "", "File Trans (*.png *.webp)");
  if (!files.isEmpty()) {
    addFiles(files);
  }
}

void KKRMainWindow::on_actionImportDirektori_triggered() {
  // import image inside single directories
  QString dir =
      QFileDialog::getExistingDirectory(this, "Pilih folder untuk diproses");
  if (!dir.isEmpty()) {
    QStringList files;
    QDirIterator iterator(dir, {"*.png", "*.webp"}, QDir::Files);
    while (iterator.hasNext()) {
      iterator.next();
      files << iterator.filePath();
    }
    addFiles(files);
  }
}

void KKRMainWindow::on_actionScan_triggered() {
  // import image on directories
  QString rootDir =
      QFileDialog::getExistingDirectory(this, "Pilih folder untuk diproses");
  if (!rootDir.isEmpty()) {
    QFileInfo rootInfo(rootDir);
    QDirIterator dirIterator(rootDir, {"*.png", "*.webp"}, QDir::Files,
                             QDirIterator::Subdirectories);
    QPixmap pix;
    bool pixOk = false;
    while (dirIterator.hasNext()) {
      dirIterator.next();
      auto info = dirIterator.fileInfo();
      auto canoPath = info.canonicalFilePath();
      if (!QPixmapCache::find(canoPath, &pix)) {
        pix = createPixmap(canoPath, &pixOk);
        if (pixOk) {
          QPixmapCache::insert(canoPath, pix);
        } else {
          emit pixmapCreationFailed(canoPath);
        }
      }
      if (!pix.isNull()) {
        auto item = new QStandardItem(info.baseName());
        item->setData(canoPath, AbsoluteFilePathRole);
        item->setData(
            pix.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation),
            Qt::DecorationRole);
        item->setData(pix, FullPixmapRole);
        item->setData(info.dir().dirName(), CustomerNameRole);
        item->setData(QColor::fromCmykF(0, 0, 0, 0, 1), Qt::BackgroundRole);
        item->setData(QString("AN: %1\nText: %2")
                          .arg(info.dir().dirName(), info.baseName()),
                      Qt::ToolTipRole);
        item->setData(fontName, FontNameRole);
        smodel->appendRow(item);
        qApp->processEvents();
      }
    }
  }
}

void KKRMainWindow::addFiles(const QStringList& listFiles) {
  for (auto filePath : listFiles) {
    qApp->processEvents();
    QFileInfo finfo(filePath);
    bool creationOk;
    QPixmap pix;
    if (!QPixmapCache::find(filePath, &pix)) {
      pix = createPixmap(filePath, &creationOk);
      // pix.save("test_.png
      if (creationOk) {
        QPixmapCache::insert(finfo.canonicalFilePath(), pix);
      } else {
        emit pixmapCreationFailed(finfo.canonicalFilePath());
      }
    }
    if (!pix.isNull()) {
      auto item = new QStandardItem(finfo.baseName());
      item->setData(finfo.canonicalFilePath(), AbsoluteFilePathRole);
      item->setData(
          pix.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation),
          Qt::DecorationRole);
      item->setData(pix, FullPixmapRole);
      item->setData(finfo.dir().dirName(), CustomerNameRole);
      item->setData(QString("AN: %1\nText: %2")
                        .arg(finfo.dir().dirName(), finfo.baseName()),
                    Qt::ToolTipRole);
      item->setData(QColor::fromCmyk(0, 0, 0, 0, 255), ColorDataRole);
      item->setData(
          ColorManagement::proof_of(QColor::fromCmyk(0, 0, 0, 0, 255)),
          Qt::BackgroundRole);
      item->setData(fontName, FontNameRole);
      smodel->appendRow(item);
    }
  }
}

QPixmap KKRMainWindow::createPixmap(const QString& fileName, bool* ok) {
  cv::Mat mat = cv::imread(fileName.toStdString(), cv::IMREAD_UNCHANGED);
  if (mat.type() != CV_8UC4) {
    mat = CVHELPER::convertTo<CV_8UC4>(mat);
  }

  if (!CVHELPER::matOk(mat)) {
    QPixmap pix(200, 200);
    pix.fill(Qt::transparent);
    if (ok) *ok = false;
    return pix;
  }

  if (ok) *ok = true;

  mat = CVHELPER::borderize(mat);

  return QPixmap::fromImage(CVHELPER::qImageFromMat(mat));
}

bool KKRMainWindow::eventFilter(QObject* watch, QEvent* ev) {
  if (watch == ui->label_4) {
    if (ev->type() == QEvent::MouseButtonPress) {
      auto me = static_cast<QMouseEvent*>(ev);
      QModelIndex ix;
      if (ui->listView->selectionModel()->hasSelection()) {
        ix = ui->listView->selectionModel()->selectedIndexes().first();
      }
      if (ix.isValid())
        emit changeBackgroudItemRequest(smodel->itemFromIndex(ix));
      me->accept();
      return true;
    }
  } else if (watch == ui->listView) {
    if (ev->type() == QEvent::KeyPress) {
      auto ke = static_cast<QKeyEvent*>(ev);
      if (ke->key() == Qt::Key_Tab) {
        auto cIndex = ui->listView->currentIndex();
        if (cIndex.isValid()) {
          auto row = cIndex.row();
          if (ke->modifiers() & Qt::ShiftModifier) {
            if (row - 1 >= 0) {
              ui->listView->setCurrentIndex(cIndex.siblingAtRow(row - 1));
            } else {
              ui->listView->setCurrentIndex(
                  cIndex.siblingAtRow(smodel->rowCount() - 1));
            }
          } else {
            if (row + 1 < smodel->rowCount()) {
              ui->listView->setCurrentIndex(cIndex.siblingAtRow(row + 1));
            } else {
              ui->listView->setCurrentIndex(smodel->index(0, 0));
            }
            return true;
          }
        }
      } else if (ke->key() == Qt::Key_Delete) {
        auto cIndex = ui->listView->currentIndex();
        if (cIndex.isValid()) {
          smodel->removeRow(cIndex.row());
          return true;
        }
      }
    } else if (ev->type() == QEvent::DragEnter) {
      auto de = static_cast<QDragEnterEvent*>(ev);
      if (de->mimeData()->hasUrls()) {
        de->acceptProposedAction();
        return true;
      }
    } else if (ev->type() == QEvent::Drop) {
      auto drop = static_cast<QDropEvent*>(ev);
      QStringList fileList;
      for (auto url : drop->mimeData()->urls()) {
        if (url.isLocalFile()) {
          auto localFile = url.toLocalFile();
          QFileInfo finfo(localFile);
          if (finfo.isDir()) {
            QDirIterator dirIterator(localFile, {"*.png", "*.webp"},
                                     QDir::Files, QDirIterator::Subdirectories);
            while (dirIterator.hasNext()) {
              dirIterator.next();
              fileList << dirIterator.fileInfo().canonicalFilePath();
            }
          } else if (finfo.isFile()) {
            QString fileName = finfo.fileName();
            if (fileName.endsWith(".png", Qt::CaseInsensitive) ||
                fileName.endsWith(".webp", Qt::CaseInsensitive)) {
              fileList << finfo.canonicalFilePath();
            }
          }
        }
      }
      if (fileList.size()) {
        addFiles(fileList);
        drop->acceptProposedAction();
        return true;
      }
    }
  }
  return QMainWindow::eventFilter(watch, ev);
}

void KKRMainWindow::changeBackgroudItemRequest(QStandardItem* item) {
  auto cl = item->data(ColorDataRole).value<QColor>();
  // cd->setOptions(QColorDialog::ShowAlphaChannel | QColorDialog::NoButtons |
  // QColorDialog::DontUseNativeDialog);
  CMYKColorDialog* cd = new CMYKColorDialog(cl, this);
  cd->setAttribute(Qt::WA_DeleteOnClose);
  connect(cd, &CMYKColorDialog::currentColorChanged, this,
          &KKRMainWindow::onColorSelected);
  cd->adjustSize();
  cd->open();
}

void KKRMainWindow::onColorSelected(const QColor& cl) {
  if (ui->listView->selectionModel()->hasSelection()) {
    auto six = ui->listView->selectionModel()->selectedIndexes().first();
    auto item = smodel->itemFromIndex(six);
    auto proof = ColorManagement::proof_of(cl);
    item->setData(proof, Qt::BackgroundRole);
    item->setData(cl, ColorDataRole);
    auto palette = ui->label_4->palette();
    palette.setColor(QPalette::Window, proof);
    ui->label_4->setPalette(palette);
    ui->label_4->setText(QString("%1\n\nKlik untuk mengubah").arg(cl.name()));
  }
}

void KKRMainWindow::onListViewSelectionChanged(const QItemSelection& sel,
                                               const QItemSelection& des) {
  if (sel.indexes().isEmpty()) {
    ui->groupBox->setDisabled(true);
    ui->label_4->setPalette(this->palette());
    ui->label_4->setText("Transparent\n\nKlik untuk mengubah");
    ui->lineEdit->blockSignals(true);
    ui->lineEdit2->blockSignals(true);
    ui->lineEdit->clear();
    ui->lineEdit2->clear();
    ui->lineEdit->blockSignals(false);
    ui->lineEdit2->blockSignals(false);
  } else {
    ui->groupBox->setEnabled(true);
    auto item = smodel->itemFromIndex(sel.indexes().first());
    auto palette = ui->label_4->palette();
    auto color = item->data(Qt::BackgroundRole).value<QColor>();
    palette.setColor(QPalette::Window, color);
    ui->label_4->setPalette(palette);
    ui->lineEdit->blockSignals(true);
    ui->lineEdit2->blockSignals(true);
    ui->lineEdit->setText(item->data(CustomerNameRole).toString());
    ui->lineEdit2->setText(item->data(InformationRole).toString());
    ui->lineEdit2->blockSignals(false);
    ui->lineEdit->blockSignals(false);
    if (color.alpha() == 0) {
      ui->label_4->setText("Transparent\n\nKlik untuk mengubah");
    } else {
      ui->label_4->setText(
          QString("%1\n\nKlik untuk mengubah").arg(color.name()));
    }
    ui->fontComboBox->blockSignals(true);
    ui->fontComboBox->setCurrentText(
        sel.indexes().first().data(FontNameRole).toString());
    ui->fontComboBox->blockSignals(false);
  }
};

void KKRMainWindow::on_lineEdit_textChanged(const QString& tx) {
  if (ui->listView->selectionModel()->hasSelection()) {
    auto idx = ui->listView->selectionModel()->selectedIndexes().first();
    if (idx.isValid()) {
      auto item = smodel->itemFromIndex(idx);
      item->setData(tx, CustomerNameRole);
    }
  }
}

void KKRMainWindow::on_lineEdit2_textChanged(const QString& tx) {
  if (ui->listView->selectionModel()->hasSelection()) {
    auto idx = ui->listView->selectionModel()->selectedIndexes().first();
    if (idx.isValid()) {
      auto item = smodel->itemFromIndex(idx);
      item->setData(tx, InformationRole);
    }
  }
}

void KKRMainWindow::on_listView_customContextMenuRequested(const QPoint& pt) {
  QMenu contextMenu(this);
  auto indexAt = ui->listView->indexAt(pt);
  if (indexAt.isValid()) {
    auto hapus = contextMenu.addAction("Hapus");
    connect(hapus, &QAction::triggered,
            [this, &indexAt]() { smodel->removeRow(indexAt.row()); });
  }
  auto reset = contextMenu.addAction("Reset");
  reset->setToolTip("Hapus semua tugas");
  connect(reset, &QAction::triggered, [this]() {
    if (QMessageBox::question(this, "Konfirmasi", "Hapus semua item?") ==
        QMessageBox::Yes) {
      smodel->clear();
    }
  });
  contextMenu.exec(ui->listView->viewport()->mapToGlobal(pt));
}

void KKRMainWindow::on_mulaiButton_clicked() {
  QStandardItem* item;
  QModelIndexList emptyCustomer, emptyInformation, emptyDrawText;
  if (smodel->rowCount() == 0) {
    QMessageBox::information(this, "Kesalahan",
                             "Belum ada data untuk di proses");
    return;
  }
  auto aData = ps.appData();
  if (aData.CounterLeft < smodel->rowCount()) {
    QMessageBox::warning(this, "Token tidak cukup",
                         QString("Token dibutuhkan : %1\nToken tersisa: %2")
                             .arg(QString::number(smodel->rowCount()))
                             .arg(QString::number(aData.CounterLeft)));
    return;
  }
  for (int r = 0; r < smodel->rowCount(); ++r) {
    auto itemIndex = smodel->index(r, 0);
    item = smodel->itemFromIndex(smodel->index(r, 0));
    if (item->data(CustomerNameRole).toString().isEmpty())
      emptyCustomer << itemIndex;
    if (item->data(InformationRole).toString().isEmpty())
      emptyInformation << itemIndex;
  }
  QString errMessage = "";
  if (!emptyCustomer.isEmpty()) {
    errMessage += QString("%1 item tidak memiliki data Konsumen\n")
                      .arg(emptyCustomer.size());
  }
  if (!emptyInformation.isEmpty()) {
    errMessage += QString("%1 item tidak memiliki Keterangan\n")
                      .arg(emptyInformation.size());
  }
  if (!emptyDrawText.isEmpty()) {
    errMessage += QString("%1 item tidak memiliki parameter Text")
                      .arg(emptyDrawText.size());
  }
  if (!errMessage.isEmpty()) {
    QMessageBox::information(this, "Data tidak lengkap",
                             QString("Error :\n%1").arg(errMessage));
    return;
  }
  auto r = new RunnableCorelTask(smodel);
  auto d = new KKRProgressDialog(this);
  r->connectSignals(d);
  auto tp = QThreadPool::globalInstance();
  d->setAttribute(Qt::WA_DeleteOnClose);
  d->setWindowFlag(Qt::FramelessWindowHint);
  r->optimization(ui->optimizerCheckBox->isChecked());
  connect(r->getN(), &Notifier::singleProcessDone,
          [=]() { ps.incrementUsage(1); });
  tp->start(r);
  d->open();
}

QWidget* KKRMainWindowInternal::Delegate::createEditor(
    QWidget* parent, const QStyleOptionViewItem& opt,
    const QModelIndex& ix) const {
  auto le = new QLineEdit(parent);
  le->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
  le->setMinimumSize(opt.rect.width(), 50);
  le->setValidator(val);
  return le;
}

void KKRMainWindow::on_aboutQt_triggered() { qApp->aboutQt(); }

void KKRMainWindow::on_aboutAutoKKR_triggered() {
  AboutKKRDialog* a = new AboutKKRDialog(&ps, this);
  a->setAttribute(Qt::WA_DeleteOnClose);
  a->open();
}

void KKRMainWindow::on_actionDialogWarna_triggered() {
  CMYKColorDialog* cd = new CMYKColorDialog(QColor::fromCmyk(0, 0, 0, 0), this);
  cd->setAttribute(Qt::WA_DeleteOnClose);
  cd->open();
}

void KKRMainWindow::onComboFontChanged(const QFont& fn) {
  if (!ui->listView->selectionModel()->hasSelection()) return;
  auto ixs = ui->listView->selectionModel()->selectedIndexes();
  auto sel = ixs.first();
  smodel->setData(sel, fn.family(), FontNameRole);
}

void KKRMainWindow::onComboFontContextMenuRequested(const QPoint& p) {
  QMenu contextMenu;
  auto currentText = ui->fontComboBox->currentText();
  auto saveDefault = contextMenu.addAction("Simpan sebagai default font");
  connect(saveDefault, &QAction::triggered,
          [this, currentText] { setDefaultFont(currentText); });
  contextMenu.exec(ui->fontComboBox->mapToGlobal(p));
}

void KKRMainWindow::setDefaultFont(const QString& _fontName) {
  QSettings s;
  s.setValue("AutoKK/defaultFont", _fontName);
  fontName = _fontName;
  s.sync();
}
