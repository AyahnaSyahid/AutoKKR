#ifndef ABOUTKKRDIALOG_H
#define ABOUTKKRDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QAction>

struct PolledSettings;
class AboutKKRDialog : public QDialog {
    Q_OBJECT

public:
  explicit AboutKKRDialog(PolledSettings *ps, QWidget *parent = nullptr);
  ~AboutKKRDialog();

private slots:
  void displayTrusts();
  void requestToken();
  void refillToken();
  
private:
  QAction *trusts;
  PolledSettings *m_ps;
};

#endif // ABOUTKKRDIALOG_H