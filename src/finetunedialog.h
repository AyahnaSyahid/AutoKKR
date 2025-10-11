#ifndef FINETUNEDIALOG_H
#define FINETUNEDIALOG_H

#include <QDialog>
#include "cv/cvhelper.hpp"

namespace Ui {
  class FineTuneDialog;
}

class FineTuneDialog : public QDialog {
  Q_OBJECT
  public:
    explicit FineTuneDialog(QWidget *parent=nullptr);
    ~FineTuneDialog();
  
  private slots:
    void handleParamChanged();
    void on_UPDATE_clicked();
    void on_lineEdit_textChanged(const QString& text);
    void on_openFile_clicked();
  
  private:
    Ui::FineTuneDialog *ui;
    QPixmap pix;
};

#endif // FINETUNEDIALOG_H