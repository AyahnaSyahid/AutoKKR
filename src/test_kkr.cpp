#include <QApplication>
#include <QLabel>
#include <QImage>
#include <QPixmap>

#include "cv/cvhelper.hpp"

QLabel *glab = nullptr;

#include <QDirIterator>
#include <QTimer>

void test() {
  QDirIterator dit("test_data", {"*.png"}, QDir::Files);
  QImage img;
  while(dit.hasNext()) {
    auto s = dit.next();
    if(not img.load(s)) {
      qDebug() << "unable to load" << s;
      continue;
    }
    qDebug() << "Processing" << s;
    auto bordered = CVHELPER::borderize(CVHELPER::matFromQImage(img));
    img = CVHELPER::qImageFromMat(bordered);
    if(glab) {
      glab->setPixmap(QPixmap::fromImage(img));
      glab->adjustSize();
    }
    img.setDotsPerMeterX(11811);
    img.setDotsPerMeterY(11811);
    img.save(QString("test_data/out/bordered_%1").arg(dit.fileName()), "", 0);
  }
}

int main(int argc, char** args) {
  QApplication app(argc, args);
  QLabel lab;
  glab = &lab;
  lab.show();
  QTimer::singleShot(200, test);
  return app.exec();
}