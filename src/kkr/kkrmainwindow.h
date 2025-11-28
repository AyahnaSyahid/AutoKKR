#ifndef KKRMAINWINDOW_H
#define KKRMAINWINDOW_H

#include <QMainWindow>
#include <QPixmap>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QItemSelection>
#include <QPoint>
// #include "countermanager.h"
#include "polledsettings.h"

namespace Ui {
  class KKRMainWindow;
}

class KKRMainWindow : public QMainWindow {
  Q_OBJECT
  
  public:
    KKRMainWindow(QWidget* parent = nullptr);
    ~KKRMainWindow();
    enum KKRDataRole {
      AbsoluteFilePathRole = Qt::UserRole + 1,
      FullPixmapRole,
      CustomerNameRole,
      DrawTextRole,
      InformationRole,
      ColorDataRole
    };
  
  private:
    QPixmap createPixmap(const QString& filePath, bool *ok);
    void addFiles(const QStringList& lFile);
    
  private slots:
    void on_actionImportFile_triggered();
    void on_actionImportDirektori_triggered();
    void on_actionScan_triggered();
    void on_lineEdit_textChanged(const QString&);
    void on_lineEdit2_textChanged(const QString&);
    void changeBackgroudItemRequest(QStandardItem* item);
    void onColorSelected(const QColor& color);
    void on_mulaiButton_clicked();
    void onListViewSelectionChanged(const QItemSelection&, const QItemSelection&);
    void on_listView_customContextMenuRequested(const QPoint& pos);
    void on_aboutQt_triggered();
    void on_aboutAutoKKR_triggered();
    void on_actionDialogWarna_triggered();

  signals:
    void pixmapCreationFailed(const QString& name);

  protected:
    bool eventFilter(QObject *watch, QEvent *ev);

  private:
    Ui::KKRMainWindow* ui;
    QStandardItemModel* smodel;
    PolledSettings ps;
};


#endif // KKRMAINWINDOW_H