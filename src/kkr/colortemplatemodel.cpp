#include "colortemplatemodel.h"
#include "colorconverter.h"
#include <QColor>
#include <QtDebug>
#include <QString>

ColorTemplateModel::ColorTemplateModel(QSettings* settings, QObject* parent)
    : QStandardItemModel(parent), m_settings(settings) {
    // Set header labels
    setHorizontalHeaderLabels({"Name", "Color"});
    // Connect signals for synchronization
    connect(this, &QStandardItemModel::dataChanged, this, &ColorTemplateModel::saveToSettings);
    connect(this, &QStandardItemModel::rowsInserted, this, &ColorTemplateModel::saveToSettings);
    connect(this, &QStandardItemModel::rowsRemoved, this, &ColorTemplateModel::saveToSettings);

    // Load initial data
    loadFromSettings();
}

ColorTemplateModel::~ColorTemplateModel() {
  saveToSettings();
};

void ColorTemplateModel::addColor(const QString& name, const QColor& c) {
  int row = rowCount();
  if(insertRows(1, 1)) {
    auto ix = index(row, 1);
    setData(ix, ColorManagement::proof_of(c), Qt::BackgroundRole);
    setData(ix, c);
    setData(ix, c.name(), Qt::EditRole);
    setData(ix.siblingAtColumn(0), name, Qt::EditRole);
  }
}

bool ColorTemplateModel::insertRows(int row, int count, const QModelIndex& parent) {
    bool success = QStandardItemModel::insertRows(row, count, parent);
    if (success) {
        for (int i = 0; i < count; ++i) {
            // Default new item: empty name, white color
            auto* nameItem = new QStandardItem("");
            QColor color = QColor::fromCmyk(0, 0, 0, 0, 255); // White, opaque
            auto* colorItem = new QStandardItem();
            colorItem->setData(ColorManagement::proof_of(color), Qt::BackgroundRole); // For display
            colorItem->setData(QVariant::fromValue(color), Qt::UserRole); // Store QColor
            colorItem->setText(color.name()); // Optional: show hex
            setItem(row + i, 0, nameItem);
            setItem(row + i, 1, colorItem);
        }
    }
    return success;
}

void ColorTemplateModel::saveToSettings() {
    if(rowCount() < 1) return;
    m_settings->beginWriteArray("colors");
    for (int row = 0; row < rowCount(); ++row) {
        m_settings->setArrayIndex(row);
        QString name = data(index(row, 0), Qt::DisplayRole).toString();
        QVariant colorVar = data(index(row, 1), Qt::UserRole);
        QColor color = colorVar.value<QColor>();
        if (color.isValid()) {
            // Store as CMYKA string
            QString colorStr = QString("%1,%2,%3,%4,%5")
                .arg(color.cyan()).arg(color.magenta())
                .arg(color.yellow()).arg(color.black())
                .arg(color.alpha());
            m_settings->setValue("color", colorStr);
        }
        m_settings->setValue("name", name);
    }
    m_settings->endArray();
    m_settings->sync(); // Ensure changes are written
}

void ColorTemplateModel::loadFromSettings() {
    clear();
    setHorizontalHeaderLabels({"Name", "Color"});
    int size = m_settings->beginReadArray("colors");
    for (int i = 0; i < size; ++i) {
        m_settings->setArrayIndex(i);
        QString name = m_settings->value("name", "").toString();
        QString colorStr = m_settings->value("color", "0,0,0,0,255").toString();
        QStringList cmyka = colorStr.split(",");
        QColor color = QColor::fromCmyk(
            cmyka[0].toInt(), cmyka[1].toInt(),
            cmyka[2].toInt(), cmyka[3].toInt(),
            cmyka[4].toInt()
        );

        auto* nameItem = new QStandardItem(name);
        auto* colorItem = new QStandardItem();
        colorItem->setData(ColorManagement::proof_of(color), Qt::BackgroundRole); // For display
        colorItem->setData(QVariant::fromValue(color), Qt::UserRole); // Store QColor
        colorItem->setText(color.name()); // Optional: show hex
        appendRow({nameItem, colorItem});
    }
    m_settings->endArray();
}