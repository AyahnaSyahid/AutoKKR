#ifndef COUNTERMANAGER_H
#define COUNTERMANAGER_H

#include <QString>
#include <QDate>
#include <QSettings>

#ifdef _WIN32
#include <windows.h>
#endif

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/sha.h>

class CounterManager {
public:
    CounterManager();
    ~CounterManager();

    bool isValid() const;
    void decrement();
    bool addFromToken(const QString& token);
    QString getInstallationID() const;
    int getCounter() const;

private:
    QString m_publicKeyPem;    // Hardcoded public key PEM
    QString m_localID;
    QString m_lastDate;
    int m_counter;
    QSettings* m_settings;

    void load();
    void save();
    QString generateID();
};

#endif // COUNTERMANAGER_H