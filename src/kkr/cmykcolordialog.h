#ifndef CMYKCOLORDIALOG_H
#define CMYKCOLORDIALOG_H

#include <QColor>
#include <QDialog>
#include <QStandardItemModel>

class ColorPaletteModel;

namespace Ui {
  class CMYKColorDialog;
}

class CMYKColorDialog : public QDialog {
  Q_OBJECT
  
  public:
    CMYKColorDialog(const QColor& c, QWidget* =nullptr);
    ~CMYKColorDialog();
    QColor getColor() const;
  
  public slots:
    void sync();
  
  private slots:
    void anySpinBoxChanged();
    void setColor(const QColor&);
    void on_terapkanButton_clicked();
    void on_addButton_clicked();
    void setLabelColor(const QColor&);
    void setSpinValues(const QColor&);
    void on_rowsInserted(const QModelIndex&, int, int);
    void on_clickedIndex(const QModelIndex& mi);
    void on_tableView_customContextMenuRequested(const QPoint&);
    void on_tableView_doubleClicked(const QModelIndex& mi);
  
  protected:
    bool eventFilter(QObject* watched, QEvent* evt) override;
  
  signals:
    void currentColorChanged(const QColor&);
  
  private:
    Ui::CMYKColorDialog* ui;
    QStandardItemModel* model;
};

#endif // CMYKCOLORDIALOG_H