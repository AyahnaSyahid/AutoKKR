#ifndef CORELDRAWCHECKER_H
#define CORELDRAWCHECKER_H



namespace CorelDrawChecker {
  struct CorelVersionCLSID {
    QString programName;
    QString CLSID;
    QString versionString;
  };
  
  const QList<CorelVersionCLSID> getVersionList();
}

#endif