#ifndef COLORTEMPLATEMODEL_H
#define COLORTEMPLATEMODEL_H

#include <QStandardItemModel>
#include <QSettings>

class ColorTemplateModel : public QStandardItemModel {
    Q_OBJECT

public:
    explicit ColorTemplateModel(QSettings* settings, QObject* parent = nullptr);
    ~ColorTemplateModel() override;
    void addColor(const QString&, const QColor&);

protected:
    bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

private slots:
    void saveToSettings();

private:
    void loadFromSettings();

    QSettings* m_settings; // Not owned, managed externally
};

#endif // COLORTEMPLATEMODEL_H