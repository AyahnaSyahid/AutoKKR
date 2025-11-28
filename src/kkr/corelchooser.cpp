#include "corelchooser.h"
#include <QSettings>
#include <QInputDialog>
#include <QDebug>

CorelChooser::CorelChooser(QObject *parent) : versionList(), corelInstalled(false), QObject(parent)
{
  QSettings lookup("HKEY_CLASSES_ROOT", QSettings::NativeFormat);
  auto a = lookup.value("CorelDRAW.Application/CLSID/.");
  if(a.isNull()) {
    return ;
  }
  corelInstalled = true;
  for(int i=20;i<31;++i) {
    auto val = lookup.value(QString("/CorelDRAW.Application.%1/CLSID/.").arg(i));
    qDebug() << val;
    if(!val.isNull()) {
      versionList << i;
    }
  }
}

CorelChooser::~CorelChooser(){}

void CorelChooser::openDialog()
{
  QList<QString> vl {"Auto"};
  for(auto x : versionList) {
    vl << QString::number(x);
  }
  auto item = QInputDialog::getItem(nullptr, "Pilih Corel", "Versi", vl, 0, false);
  if(!item.isEmpty()) {
    QSettings s("BlackCircle", "AutoKKR");
    s.setValue("CorelDRAW/UseVersion", item);
    s.sync();
  }
}