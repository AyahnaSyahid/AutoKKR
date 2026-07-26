#ifndef RUNNABLECORELTASK_H
#define RUNNABLECORELTASK_H

#include "notifier.h"
#include <QRunnable>
#include <QAbstractItemModel>

class QAxObject;
class RunnableCorelTask : public QRunnable {

  public:
    RunnableCorelTask(QAbstractItemModel *model);
    void run() override;
    bool connectSignals(QObject *obj);
    inline const Notifier *getN() const { return &n; };
    void optimization(bool = true);

  private:
    bool optimize;
    QAbstractItemModel *taskModel;
    Notifier n;
    bool connected;
    QList<QSizeF> duplicateOffsets;
    void addInfoText(QAxObject *layer, QAxObject *pageRect, const QString& ket);
    void colorizePageRect(QAxObject *pageRect, const QColor& cl);
};

#endif // RUNNABLECORELTASK_H