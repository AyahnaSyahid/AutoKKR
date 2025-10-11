#ifndef REQUESTPARSER_H
#define REQUESTPARSER_H

#include <QDialog>

namespace Ui {
  class RequestParserDialog;
}

class RequestParser : public QDialog {
  Q_OBJECT

public:
  explicit RequestParser(QWidget *parent);
  ~RequestParser();

private slots:
  void on_loadButton_clicked();
  void on_testButton_clicked();
  void on_okButton_clicked();

private:
  Ui::RequestParserDialog *ui;
};

#endif // REQUESTPARSER_H