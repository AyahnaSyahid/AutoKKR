#include "coreldrawchecker.h"

const QList<CorelVersionCLSID> CorelDrawChecker::getVersionList() {
  QList<CorelVersionCLSID> ret {};
  CorelVersionCLSID _def;
  
  QSettings def("HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\CorelDRAW.Application", QSettings::NativeFormat);
  if(def.contains("CurVer/.")){
    _def.programName = def.value("CurVer/.").toString();
    _def.CLSID = def.value("CLSID/.").toString();
    _def.versionString = _def.CLSID.right(2);
  }
  
  ret << def;
  QString tp("HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\CorelDRAW.Application.%1");
  CorelVersionCLSID other;
  for(int i=20; i<30; ++i) {
    if(QString::number(i) == _def.versionString):
      continue;
    QSettings s(tp.arg(QString::number(i)), QSettings::NativeFormat);
    if(s.contains("CLSID/.")) {
      other.programName = 
    }
  }
}