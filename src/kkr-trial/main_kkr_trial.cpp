#include "cv/cvhelper.hpp"
#include "kkr/kkrmainwindow.h"
#include <QApplication>
#include <QSettings>
#include <QDate>
#include <QMenu>
#include <QMessageBox>

int main(int argc, char** args) {
  QApplication app(argc, args);
  app.setApplicationName("Auto KKR (DEV)");
  app.setOrganizationName("BlackCircle");
  KKRMainWindow kkrm;
  kkrm.setWindowTitle(QString("%1 - ( Versi DEV 19/9/2025 )").arg(kkrm.windowTitle()));
  if(QDate::currentDate() > QDate::fromString("2025-10-03", "yyyy-MM-dd")) {
    kkrm.show();
    QMessageBox::critical(nullptr, "Trial Berakhir", "Terimakasih telah menjadi penguji software kami");
    app.quit();
    return 0;
  }
  kkrm.show();
  return app.exec();
}