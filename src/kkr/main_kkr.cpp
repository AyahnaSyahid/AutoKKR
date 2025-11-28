#include "cv/cvhelper.hpp"
#include "kkrmainwindow.h"
#include <QApplication>

int main(int argc, char** args) {
  QApplication app(argc, args);
  qSetMessagePattern("%{file}(%{line}): %{message}");
  app.setOrganizationName("BlackCircle");
  app.setApplicationName("AutoKKR");
  app.setApplicationVersion("1.2.1");
  KKRMainWindow kkrm;
  kkrm.show();
  return app.exec();
}