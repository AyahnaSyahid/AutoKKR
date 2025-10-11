#ifndef NOTIFIER_H
#define NOTIFIER_H

#include <QObject>

class Notifier : public QObject {
  Q_OBJECT
  
  public:
    Notifier();
    void emitCurrent(int);
    void emitMaximum(int);
    void emitMinimum(int);
    void emitMessage(const QString &);
    inline bool isCanceled() const { return _cancel; }
  
  public slots:
    void cancelRequested();

  signals:
    void singleProcessDone();
    void currentChanged(int c);
    void minimumChanged(int m);
    void maximumChanged(int m);
    void error(const QString &msg);
    void started();
    void canceled();
    void stopped();
    void messageChanged(const QString &msg);

  private:
    bool _cancel;

};

#endif // NOTIFIER_H