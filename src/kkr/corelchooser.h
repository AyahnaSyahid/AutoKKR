#ifndef CORELCHOOSER_H
#define CORELCHOOSER_H

#include <QObject>

class CorelChooser : public QObject {
  Q_OBJECT

public:
  CorelChooser(QObject *parent=nullptr);
  ~CorelChooser();

public slots:
  void openDialog();

private:
  QList<int> versionList;
  bool corelInstalled;
};

#endif