#include "runnablecoreltask.h"

#include <objbase.h>
#include <windows.h>

#include <QApplication>
#include <QAxObject>
#include <QDir>
#include <QTemporaryDir>
#include <QThread>
#include <QtDebug>
#include <QtMath>

#include "kkrmainwindow.h"
#include "kkrprogressdialog.h"

RunnableCorelTask::RunnableCorelTask(QAbstractItemModel* model)
    : taskModel(model),
      n(),
      connected(false),
      optimize(false),
      duplicateOffsets{},
      QRunnable() {
  qreal offsets = 61.297, my, mx;

  for (int r = 0; r < 8; ++r) {
    for (int c = 0; c < 5; ++c) {
      if (r == 0 && c == 0) continue;
      if (c % 2) {
        duplicateOffsets << QSizeF(c * offsets, (-r * offsets) + 35.867);
        if (r == 7) {
          duplicateOffsets << QSizeF(c * offsets,
                                     (-r * offsets) + 35.867 - offsets);
        }
      } else {
        duplicateOffsets << QSizeF(c * offsets, -r * offsets);
      }
    }
  }
}

bool RunnableCorelTask::connectSignals(QObject* obj) {
  KKRProgressDialog* kkr_pd = qobject_cast<KKRProgressDialog*>(obj);
  if (!kkr_pd) return false;
  n.connect(&n, &Notifier::currentChanged, kkr_pd,
            &KKRProgressDialog::setCurrent);
  n.connect(&n, &Notifier::maximumChanged, kkr_pd,
            &KKRProgressDialog::setMaximum);
  n.connect(&n, &Notifier::minimumChanged, kkr_pd,
            &KKRProgressDialog::setMinimum);
  n.connect(&n, &Notifier::messageChanged, kkr_pd,
            &KKRProgressDialog::setMessage);
  n.connect(&n, &Notifier::started, kkr_pd, &KKRProgressDialog::onStarted);
  n.connect(&n, &Notifier::canceled, kkr_pd, &KKRProgressDialog::onCanceled);
  n.connect(&n, &Notifier::stopped, kkr_pd, &KKRProgressDialog::onStopped);
  n.connect(&n, &Notifier::error, kkr_pd, &KKRProgressDialog::onError);
  n.connect(kkr_pd, &KKRProgressDialog::requestCancel, &n,
            &Notifier::cancelRequested);
  connected = true;
  return connected;
}

void RunnableCorelTask::run() {
  CoInitializeEx(NULL, COINIT_MULTITHREADED);
  emit n.started();
  QAxObject ax;
  if (optimize) {
    // ax.disableMetaObject();
    // ax.disableEventSink();
    qDebug() << "Optimization Enabled";
  }
  QSettings settings("BlackCircle", "AutoKKR");
  if (settings.contains("CorelDRAW/UseVersion")) {
    QString version = settings.value("CorelDRAW/UseVersion").toString();
    if (version == "Auto") {
      ax.setControl("CorelDRAW.Application");
    } else {
      ax.setControl(QString("CorelDRAW.Application.%1").arg(version));
    }
  } else if (settings.contains("CorelDRAW/LastUsedVersion")) {
    QString version = settings.value("CorelDRAW/LastUsedVersion").toString();
    ax.setControl(QString("CorelDRAW.Application.%1").arg(version));
  }

  if (ax.isNull()) {
    QSettings cs("HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\CorelDRAW.Application",
                 QSettings::NativeFormat);
    if (!cs.contains("CLSID/.")) {
      n.emitMessage("Tidak dapat menginisialisasi CorelDRAW");
      CoUninitialize();
      emit n.stopped();
      return;
    }
    QString clsid = cs.value("CLSID/.", "").toString();
    ax.setControl(clsid);
  }

  if (optimize) {
    // ax.setProperty("Visible", false);
    ax.setProperty("Visible", false);
    ax.setProperty("Optimization", true);
    ax.setProperty("EventsEnabled", false);
  }
  n.emitMessage("Inisiasi CorelDRAW berhasil!");
  // auto current_winstate =
  // ax.querySubObject("ActiveWindow")->property("WindowState").toInt();
  // qDebug() << "CurrentWindowState = " << current_winstate;
  // ax.querySubObject("ActiveWindow")->setProperty("WindowState", 2);
  // qDebug() << "CurrentWindowState = " << current_winstate;
  n.emitMessage("Tunggu sebentar . . .");
  emit n.messageChanged("Memulai !");

  QTemporaryDir tempDir;
  int itemCount = taskModel->rowCount(), failCount = 0;
  n.emitMaximum(itemCount);
  auto sio = ax.querySubObject("CreateStructImportOptions()");
  for (int i = 0; i < itemCount; ++i) {
    QString savedName =
        tempDir.filePath(QString("pix_%1.png").arg(i, 2, 10, QChar('0')));
    auto modelIndex = taskModel->index(i, 0);
    QPixmap pix =
        modelIndex.data(KKRMainWindow::FullPixmapRole).value<QPixmap>();
    if (pix.isNull()) {
      ++failCount;
      n.emitMessage("Tidak dapat membaca gambar");
      n.emitMinimum(failCount);
      continue;
    }
    savedName = QDir::toNativeSeparators(
        modelIndex.data(KKRMainWindow::AbsoluteFilePathRole).toString());
    n.emitMessage("Membuat dokumen baru. . .");
    auto doc = ax.querySubObject("CreateDocument()");
    // QThread::sleep(1);
    doc->setProperty("Unit", 3);
    doc->setProperty("ReferencePoint", 9);
    doc->setProperty(
        "Name", modelIndex.data(KKRMainWindow::InformationRole).toString());
    auto page = doc->querySubObject("ActivePage");
    page->dynamicCall("SetSize(330, 483)");
    auto layer = page->querySubObject("Layers(\"Layer 1\")");
    n.emitMessage("Mengimport gambar. . .");
    layer->dynamicCall("Import(QString, int, QAxObject)", savedName, 0,
                       sio->asVariant());
    auto picShape = layer->querySubObject("Shapes")->querySubObject("First");
    n.emitMessage("Mengextract alpha . . .");
    auto alpha_image =
        picShape->querySubObject("Bitmap")->querySubObject("ImageAlpha");
    bool alphaImageFound = false;
    if (!alpha_image->isNull()) {
      n.emitMessage("Alpha channel ditemukan");
      alphaImageFound = true;
      auto alpha_image_c = alpha_image->dynamicCall("GetCopy()");
      // qDebug() << alpha_image_c;
      QVariantList createBitmapParams = {picShape->property("LeftX"),
                                         picShape->property("TopY"),
                                         picShape->property("RightX"),
                                         picShape->property("BottomY"),
                                         alpha_image_c,
                                         QVariant()};

      auto alpha_shape = layer->querySubObject(
          "CreateBitmap(double, double, double, double, IDispatch*, "
          "IDispatch*)",
          createBitmapParams);
      n.emitMessage("Mengukir alpha channel");
      auto traceSettings =
          alpha_shape->querySubObject("Bitmap")->querySubObject("Trace(2)");
      traceSettings->dynamicCall("SetColorMode(8)");
      traceSettings->setProperty("BackgroundRemovalMode", 2);
      // property ini readonly
      // traceSettings->querySubObject("BackgroundColor")->dynamicCall("BWAssign(0)");
      traceSettings->setProperty("DeleteOriginalObject", true);
      traceSettings->setProperty("RemoveEntireBackColor", true);
      traceSettings->dynamicCall("ApplyChanges()");
      traceSettings->dynamicCall("Finish()");
      auto traced = layer->querySubObject("Shapes")->querySubObject("First");
      // cek apakah traced adalah group shape
      if (traced->property("Type").toInt() == 7) {
        QVariantList fsParam{QVariant(), 0, true,
                             "@fill.color.iswhite() = False"};
        auto traced_ungrouped = traced->querySubObject("UngroupEx()");
        traced_ungrouped->querySubObject("Shapes")
            ->querySubObject(
                "FindShapes(const QString&, int, bool, const QString&)",
                fsParam)
            ->dynamicCall("Delete()");
        traced = doc->querySubObject("ActiveShape");
      }
      qreal maxApprox = qMax(traced->property("SizeWidth").toDouble(),
                             traced->property("SizeHeight").toDouble());
      auto tracedContour = traced->querySubObject("Curve")->querySubObject(
          "Contour(qreal, int, int, int)", maxApprox / 15.0, 1, 1, 2);
      tracedContour->dynamicCall("AutoReduceNodes(15)");
      auto contourShape = layer->querySubObject("CreateCurve(&QAxObject)",
                                                tracedContour->asVariant());
      contourShape->querySubObject("Fill")
          ->querySubObject("UniformColor")
          ->dynamicCall("BWAssign(1)");
      contourShape->querySubObject("Outline")->dynamicCall("SetNoOutline()");
      picShape->querySubObject("Bitmap")->dynamicCall(
          "ConvertTo(5)");  // As CMYK Image
      picShape->dynamicCall("AddToPowerClip(&QAxObject)",
                            contourShape->asVariant());
      picShape = contourShape;
      traced->dynamicCall("Delete()");
    } else {
      n.emitMessage("Alpha channel tidak ditemukan . . .");
    }
    n.emitCurrent(i + 1);
    qreal width = picShape->property("SizeWidth").toDouble(),
          height = picShape->property("SizeHeight").toDouble();
    QSizeF picSize(width, height);
    QString itemText = modelIndex.data(Qt::DisplayRole).toString();
    if (itemText.isEmpty()) {         // Tidak disertai Text
      picSize.scale(51.0, 51.0, Qt::KeepAspectRatio);
      picShape->dynamicCall("SetSize(qreal, qreal)", picSize.width(),
                            picSize.height());
      picShape->dynamicCall("SetPosition(qreal, qreal)", 42.406, 450.821);
      auto shapeRange = ax.querySubObject("CreateShapeRange()");
      shapeRange->dynamicCall("Add(QAxObject)", picShape->asVariant());
      for (auto off : duplicateOffsets) {
        auto cs = picShape->querySubObject("Duplicate(qreal, qreal)",
                                           off.width(), off.height());
        shapeRange->dynamicCall("Add(QAxObject)", cs->asVariant());
      }
      auto pageRect = layer->querySubObject(
          "CreateRectangle(qreal, qreal, qreal, qreal,)",
          page->property("LeftX"), page->property("TopY"),
          page->property("RightX"), page->property("BottomY"));
      pageRect->querySubObject("Outline")->dynamicCall("SetNoOutline()");
      auto itemColor =
          modelIndex.data(KKRMainWindow::ColorDataRole).value<QColor>();
      colorizePageRect(pageRect, itemColor);
      pageRect->dynamicCall("OrderToBack()");
      shapeRange->dynamicCall("AddToPowerClip(QAxObject)",
                              pageRect->asVariant());
      addInfoText(layer, pageRect, modelIndex.data(KKRMainWindow::InformationRole).toString());
      shapeRange->clear();
      pageRect->clear();
    } else {                          // Disertai Text
      picSize.scale(38.0, 38.0, Qt::KeepAspectRatio);
      picShape->dynamicCall("SetSize(qreal, qreal)", picSize.width(),
                            picSize.height());
      picShape->dynamicCall("SetPosition(qreal, qreal)", 42.619, 457.858);
      auto outerRect = layer->querySubObject(
          "CreateRectangle(qreal, qreal, qreal, qreal)",
          picShape->property("LeftX").toDouble() - 12.398,
          picShape->property("TopY").toDouble() + 5.149,
          picShape->property("RightX").toDouble() + 12.398,
          picShape->property("BottomY").toDouble() - 19.222);
      outerRect->querySubObject("Outline")->dynamicCall("SetNoOutline()");
      QList<QVariant> vmap;
      vmap << outerRect->property("LeftX").toDouble() + 3.0;       // Left
      vmap << outerRect->property("BottomY").toDouble() + 17.942;  // Top
      vmap << outerRect->property("RightX").toDouble() - 3.0;      // Right
      vmap << outerRect->property("BottomY").toDouble() + 1.097;   // Bottom
      vmap << modelIndex.data(Qt::DisplayRole);                    // Text
      vmap << 0;                                                   // LanguageID
      vmap << -1;                                                  // CharSet
      vmap << modelIndex.data(
          KKRMainWindow::KKRDataRole::FontNameRole);  // Font
      vmap << 15;                                     // Size
      vmap << -2;                                     // Bold
      vmap << -2;                                     // Italic
      vmap << 7;                                      // Underline
      vmap << 3;                                      // Alignment

      auto textShape = layer->querySubObject(
          "CreateParagraphText(qreal, qreal, qreal, qreal, const QString&, "
          "int, int, "
          "const QString& , int, int, int, int, int)",
          vmap);
      auto textFill = textShape->querySubObject("Fill")
                          ->querySubObject("UniformColor")
                          ->dynamicCall("CMYKAssign(0, 0, 0, 0)");

      auto textOutline = textShape->querySubObject("Outline");
      textOutline->querySubObject("Color")->dynamicCall(
          "CMYKAssign(int c, int m, int y, int k)", 80, 40, 40, 100);
      textOutline->setProperty("Width", 1.5471);
      textOutline->setProperty("LineCaps", 1);
      textOutline->setProperty("BehindFill", 1);
      textOutline->setProperty("LineJoin", 1);

      auto frame = textShape->querySubObject("Text")
                       ->querySubObject("Story")
                       ->querySubObject("Frames(1)");
      frame->setProperty("VerticalAlignment", 1 /* CenterJustify */);
      auto story = textShape->querySubObject("Text")->querySubObject("Story");
      // story->setProperty("LineSpacingType", 0);
      // story->setProperty("LineSpacing", 100.0);

      story->dynamicCall("SetLineSpacing(int, qreal, int, int)", 0, 100.0, -1,
                         -1);

      textShape->querySubObject("Text")->dynamicCall("FitTextToFrame()");
      auto shapeRange = ax.querySubObject("CreateShapeRange()");
      shapeRange->dynamicCall("Add(QAxObject)", picShape->asVariant());
      shapeRange->dynamicCall("Add(QAxObject)", textShape->asVariant());
      shapeRange->dynamicCall("AddToPowerClip(QAxObject)",
                              outerRect->asVariant());
      shapeRange->dynamicCall("RemoveAll()");

      picShape->clear();
      textShape->clear();

      for (auto ds : duplicateOffsets) {
        auto duplicate = outerRect->querySubObject("Duplicate(qreal, qreal)",
                                                   ds.width(), ds.height());
      }

      shapeRange = layer->querySubObject("Shapes")->querySubObject("All()");

      auto pageRect = layer->querySubObject(
          "CreateRectangle(qreal, qreal, qreal, qreal)",
          page->property("LeftX").toDouble(), page->property("TopY").toDouble(),
          page->property("RightX").toDouble(),
          page->property("BottomY").toDouble());

      shapeRange->dynamicCall("AddToPowerClip(QAxObject)",
                              pageRect->asVariant());
      outerRect->clear();
      auto itemColor =
          modelIndex.data(KKRMainWindow::ColorDataRole).value<QColor>();
      colorizePageRect(pageRect, itemColor);
      pageRect->dynamicCall("OrderToBack()");
      pageRect->querySubObject("Outline")->dynamicCall("SetNoOutline()");
      addInfoText(layer, pageRect, modelIndex.data(KKRMainWindow::InformationRole).toString());
    }
    emit n.singleProcessDone();
  }
  if (optimize) {
    ax.setProperty("EventsEnabled", true);
    ax.setProperty("Optimization", false);
  }
  ax.setProperty("Visible", true);
  ax.querySubObject("ActiveWindow")->dynamicCall("Refresh()");
  CoUninitialize();
  emit n.stopped();
}

void RunnableCorelTask::optimization(bool n) { optimize = n; }

void RunnableCorelTask::addInfoText(QAxObject* layer, QAxObject* pageRect,
                                    const QString& ket) {
  auto infoText = layer->querySubObject("CreateArtisticText(qreal, qreal, QString&)", 0.0, 0.0, ket);
  infoText->querySubObject("Fill")->querySubObject("UniformColor")->dynamicCall("GrayAssign(int)", 84);
  auto story = infoText->querySubObject("Text")->querySubObject("Story");
  story->setProperty("Font", "Segoe UI");
  story->setProperty("Size", 8);
  infoText->setProperty("RightX", 323);
  infoText->setProperty("TopY", 478);
}

void RunnableCorelTask::colorizePageRect(QAxObject* pageRect,
                                         const QColor& cl)
{
  bool fullTransparent = cl.alpha() == 0;
  bool fullOpaque = cl.alpha() == 255;
  auto fill = pageRect->querySubObject("Fill");
  if (fullTransparent) {
    fill->dynamicCall("ApplyNoFill()");
    return ;
  }
  auto uniColor = fill->querySubObject("UniformColor");
  if (cl.spec() == QColor::Spec::Cmyk) {
    uniColor->dynamicCall("CMYKAssign(int, int, int, int)", 
        qRound(cl.cyan() / 255.0) * 100,
        qRound(cl.magenta() / 255.0) * 100,
        qRound(cl.yellow() / 255.0) * 100,
        qRound(cl.black() / 255.0) * 100 );
  } else {
    uniColor->dynamicCall("RGBAssign(int, int, int)", 
        cl.red(), cl.green(), cl.blue() );
  }
  auto transparency = pageRect->querySubObject("Transparency");
  if (!fullOpaque) {
    if (transparency) {
      transparency->dynamicCall("ApplyUniformTransparency(int)", qRound(cl.alphaF() * 100));
    }
  } else {
    if (transparency) {
      transparency->dynamicCall("ApplyNoTransparency()");
    }
  }
}
