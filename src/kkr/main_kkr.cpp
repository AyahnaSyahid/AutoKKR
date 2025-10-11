#include "cv/cvhelper.hpp"
#include "kkrmainwindow.h"
#include <QApplication>

int main(int argc, char** args) {
  QApplication app(argc, args);
  qSetMessagePattern("%{file}(%{line}): %{message}");
  app.setApplicationName("AutoKKR");
  app.setOrganizationName("BlackCircle");
  app.setApplicationVersion("1.2.1");
  KKRMainWindow kkrm;
  kkrm.show();
  return app.exec();
}