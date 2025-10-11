#ifndef COUNTERGENWINDOW_H
#define COUNTERGENWINDOW_H

#include <QMainWindow>

namespace Ui {
  class CounterGenWindow;
};

using GeneratorPtr = QByteArray (*)(const QString&, int val);

class CounterGenWindow : public QMainWindow {
  Q_OBJECT
public:
  explicit CounterGenWindow(QWidget* parent=nullptr);
  ~CounterGenWindow();
  void setGenerator(GeneratorPtr g);

private slots:
  void on_buatButton_clicked();
  void on_simpanButton_clicked();
  void on_loadButton_clicked();
  
private:
  QByteArray generated;
  GeneratorPtr gen;
  Ui::CounterGenWindow *ui;
};



#endif // COUNTERGENWINDOW_H