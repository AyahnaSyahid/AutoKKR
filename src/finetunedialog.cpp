#include "finetunedialog.h"
#include "ui_finetunedialog.h"
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QComboBox>
#include <QFileDialog>

FineTuneDialog::FineTuneDialog(QWidget *parent)
: ui(new Ui::FineTuneDialog), pix(), QDialog(parent) {
  ui->setupUi(this);
  
  ui->GPB->setItemData(ui->GPB->findText("REPLICATE"), 
           cv::BORDER_REPLICATE, 8888);
  ui->GPB->setItemData(ui->GPB->findText("CONSTANT"), 
           cv::BORDER_CONSTANT, 8888);
  ui->GPB->setItemData(ui->GPB->findText("REFLECT"), 
           cv::BORDER_REFLECT, 8888);
  ui->GPB->setItemData(ui->GPB->findText("WRAP"), 
           cv::BORDER_WRAP, 8888);
  ui->GPB->setItemData(ui->GPB->findText("REFLECT 101"), 
           cv::BORDER_REFLECT_101, 8888);
  ui->GPB->setItemData(ui->GPB->findText("TRANSPARENT"), 
           cv::BORDER_TRANSPARENT, 8888);
  
  ui->EPB->setItemData(ui->EPB->findText("REPLICATE"), 
           cv::BORDER_REPLICATE, 8888);
  ui->EPB->setItemData(ui->EPB->findText("CONSTANT"), 
           cv::BORDER_CONSTANT, 8888);
  ui->EPB->setItemData(ui->EPB->findText("REFLECT"), 
           cv::BORDER_REFLECT, 8888);
  ui->EPB->setItemData(ui->EPB->findText("WRAP"), 
           cv::BORDER_WRAP, 8888);
  ui->EPB->setItemData(ui->EPB->findText("REFLECT 101"), 
           cv::BORDER_REFLECT_101, 8888);
  ui->EPB->setItemData(ui->EPB->findText("TRANSPARENT"), 
           cv::BORDER_TRANSPARENT, 8888);
  
  ui->EPM->setItemData(ui->EPM->findText("ELLIPSE"),
          cv::MORPH_ELLIPSE, 8888);
  ui->EPM->setItemData(ui->EPM->findText("CROSS"),
          cv::MORPH_CROSS, 8888);
  ui->EPM->setItemData(ui->EPM->findText("RECT"),
          cv::MORPH_RECT, 8888);
  
  setProperty("GPK1", 1);
  setProperty("GPK2", 1);
  setProperty("GPS1", 0.0);
  setProperty("GPS2", 0.0);
  setProperty("GPB", ui->GPB->currentData(8888));
  setProperty("EPB", ui->EPB->currentData(8888));
  setProperty("EPM", ui->EPM->currentData(8888));
  setProperty("EPS1", 1);
  setProperty("EPS2", 1);
  setProperty("EPK1", 1);
  setProperty("EPK2", 1);
  setProperty("EPI", 1);
}

FineTuneDialog::~FineTuneDialog() {
  delete ui;
}

void FineTuneDialog::handleParamChanged() {
  auto sdr = sender();
  if(sdr == ui->GPK1) {
    auto spin = qobject_cast<QSpinBox*>(sdr);
    if(spin) {
      setProperty("GPK1", spin->value());
    }
  } else if(sdr == ui->GPK2) {
      auto spin = qobject_cast<QSpinBox*>(sdr);
      if(spin) {
        setProperty("GPK2", spin->value());
      }
  } else if(sdr == ui->GPS1) {
      auto spin = qobject_cast<QDoubleSpinBox*>(sdr);
      if(spin) {
        setProperty("GPS1", spin->value());
      }
  } else if ( sdr == ui->GPS2) {
      auto spin = qobject_cast<QDoubleSpinBox*>(sdr);
      if(spin) {
        setProperty("GPS2", spin->value());
      }
  } else if(sdr == ui->EPS1) {
      auto spin = qobject_cast<QSpinBox*>(sdr);
      if(spin) {
        setProperty("EPS1", spin->value());
      }
  }  else if(sdr == ui->EPS2) {
      auto spin = qobject_cast<QSpinBox*>(sdr);
      if(spin) {
        setProperty("EPS2", spin->value());
      }
  } else if(sdr == ui->EPK1) {
      auto spin = qobject_cast<QSpinBox*>(sdr);
      if(spin) {
        setProperty("EPK1", spin->value());
      }
  } else if(sdr == ui->EPK2) {
      auto spin = qobject_cast<QSpinBox*>(sdr);
      if(spin) {
        setProperty("EPK2", spin->value());
      }
  } else if(sdr == ui->EPI) {
      auto spin = qobject_cast<QSpinBox*>(sdr);
      if(spin) {
        setProperty("EPI", spin->value());
      }
  } else if(sdr == ui->EPB) {
      auto combo = qobject_cast<QComboBox*>(sdr);
      if(combo) {
        setProperty("EPB", combo->currentData(8888));
      }
  } else if(sdr == ui->EPM) {
      auto combo = qobject_cast<QComboBox*>(sdr);
      if(combo) {
        setProperty("EPM", combo->currentData(8888));
      }
  } else if(sdr == ui->GPB) {
      auto combo = qobject_cast<QComboBox*>(sdr);
      if(combo) {
        setProperty("GPB", combo->currentData(8888));
      }
  }
}

using contour_t = std::vector<cv::Point>;
using contours_t = std::vector<contour_t>;

void FineTuneDialog::on_UPDATE_clicked() {
    if(ui->lineEdit->text().isEmpty()) {
      ui->IML->setPixmap(QPixmap());
      ui->IML->setText("No IMAGE");
      return;
    } 
    QImage src(ui->lineEdit->text());
    if(src.isNull()) {
      ui->IML->setPixmap(QPixmap());
      ui->IML->setText("Gagal membuka file :" + ui->lineEdit->text());
      return;
    }
    auto aa = CVHELPER::matFromQImage(src);
    
    auto img = CVHELPER::cropToAlpha(aa);
    auto imgSize = img.size();

    // Calculate the border size based on BORDER_TO_IMAGE_RATIO
    int borderSize = (int)(std::max(imgSize.width, imgSize.height) * CVHELPER::BORDER_TO_IMAGE_RATIO * 2);
    int maximized = std::max(imgSize.width, imgSize.height) + borderSize;
    auto requiredSize = cv::Size(maximized, maximized);
    // qDebug() << "Add Border Width" << borderSize;

    // Create transparent matrix
    cv::Mat transparent(requiredSize, img.type(), cv::Scalar(255, 255, 255, 0));

    // Center the image in the transparent matrix
    int x_offset = (maximized - imgSize.width) / 2;
    int y_offset = (maximized - imgSize.height) / 2;

    // Ensure offsets are non-negative and ROI is valid
    x_offset = std::max(0, x_offset);
    y_offset = std::max(0, y_offset);
    cv::Rect roi(x_offset, y_offset, img.cols, img.rows);

    // Check if ROI is within bounds
    if (roi.x + roi.width > transparent.cols || roi.y + roi.height > transparent.rows) {
        throw std::runtime_error("ROI exceeds transparent matrix dimensions");
    }

    cv::Mat transROI = transparent(roi);
    img.copyTo(transROI);
    img = transparent;

    // Rest of your code remains unchanged
    cv::Mat alpha_source;
    cv::extractChannel(img, alpha_source, 3);

    imgSize = img.size();

    contours_t contours;
    cv::findContours(alpha_source, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_TC89_KCOS);
    cv::Mat newBG(imgSize, img.type(), cv::Scalar(255, 255, 255, 0));
    cv::drawContours(newBG, contours, -1, cv::Scalar(255, 255, 255, 255),
                      maximized / CVHELPER::BORDER_PIXEL_TO_IMAGE_RATIO, cv::FILLED);
    // qDebug() << "Border Brush" << maximized / BORDER_PIXEL_TO_IMAGE_RATIO;
    cv::drawContours(newBG, contours, -1, cv::Scalar(255, 255, 255, 255), cv::FILLED);
    
    // showInCheckerBoard(newBG, "newBG");
    auto last = CVHELPER::blend(newBG, img);

    cv::Mat last_alpha, blured_alpha;
    cv::extractChannel(newBG, last_alpha, 3);
    int GPB = property("GPB").toInt(),
        GPK1 = property("GPK1").toInt(),
        GPK2 = property("GPK2").toInt(),
        EPM = property("EPM").toInt(),
        EPS1 = property("EPS1").toInt(),
        EPS2 = property("EPS2").toInt(),
        EPK1 = property("EPK1").toInt(),
        EPK2 = property("EPK2").toInt(),
        EPI = property("EPI").toInt(),
        EPB = property("EPB").toInt();
    qreal GPS1 = property("GPS1").toDouble(),
          GPS2 = property("GPS1").toDouble();
    cv::GaussianBlur(last_alpha, blured_alpha, {GPK1, GPK2}, GPS1, GPS2, GPB);
    cv::erode(blured_alpha, blured_alpha, cv::getStructuringElement(EPM, cv::Size(EPS1, EPS2)), {-1, -1}, EPI, EPB);
    // cv::dilate(last_alpha, last_alpha, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3)), {-1, -1}, 7, cv::BORDER_REPLICATE);
    std::vector<cv::Mat> fixed;
    cv::split(last, fixed);
    cv::Mat onlyRGB;

    fixed[3] = blured_alpha;
    cv::merge(fixed, last);
    
    auto qimg_last = CVHELPER::qImageFromMat(last);
    ui->IML->setPixmap(QPixmap::fromImage(qimg_last).scaled(ui->IML->geometry().size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}


void FineTuneDialog::on_lineEdit_textChanged(const QString& text) {
  Q_UNUSED(text);
  ui->UPDATE->click();
}

void FineTuneDialog::on_openFile_clicked() {
  auto openFile = QFileDialog::getOpenFileName(this, "Pilih Sample", "", "PNG File (*.png)");
  if(!openFile.isEmpty()) {
    ui->lineEdit->setText(openFile);
  }
}

#include <QApplication>

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  FineTuneDialog ftd;
  ftd.open();
  return app.exec();
}