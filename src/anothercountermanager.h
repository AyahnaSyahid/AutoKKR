#ifndef COUNTERMANAGER_H
#define COUNTERMANAGER_H

#include <QObject>
#include <QSettings>
#include <QString>
#include <QDateTime>

class CounterManager : public QObject {
    Q_OBJECT
public:
    explicit CounterManager(QObject *parent = nullptr);
    ~CounterManager();

    bool isValid() const;
    void decrement();
    bool addFromToken(const QString& token);
    QString getInstallationID() const;
    int getCounter() const;

signals:
    void counterError(const QString& error);

private:
    struct AppSettingsData {
        QString machineID;
        QString appID;
        int usageCounter;
        int counterLeft;
        QDateTime lastRefill;
        int version;

        bool isValid() const {
            return !machineID.isEmpty() && !appID.isEmpty() && lastRefill.isValid() && version >= 0;
        }
        QString serialize() const {
            return QString("%1:%2:%3:%4:%5:%6")
                .arg(machineID)
                .arg(appID)
                .arg(usageCounter)
                .arg(counterLeft)
                .arg(lastRefill.toString(Qt::ISODate))
                .arg(version);
        }
        static AppSettingsData deserialize(const QString& data) {
            AppSettingsData result;
            QStringList parts = data.split(":");
            if (parts.size() == 6) {
                result.machineID = parts[0];
                result.appID = parts[1];
                result.usageCounter = parts[2].toInt();
                result.counterLeft = parts[3].toInt();
                result.lastRefill = QDateTime::fromString(parts[4], Qt::ISODate);
                result.version = parts[5].toInt();
            }
            return result;
        }
        void print() const {
            qDebug() << "MachineID:" << machineID
                     << "AppID:" << appID
                     << "UsageCounter:" << usageCounter
                     << "CounterLeft:" << counterLeft
                     << "LastRefill:" << lastRefill
                     << "Version:" << version;
        }
    };

    static bool resources_initialized;
    QString m_publicKeyPem;
    QString m_privateKeyPem;
    AppSettingsData m_data;
    QList<QSettings*> m_settingsList;

    void load();
    void save();
    QString generateID();
    void initializeSettings();
    QByteArray encryptData(const QString& data) const;
    QString decryptData(const QByteArray& encrypted) const;
};

#endif // COUNTERMANAGER_H