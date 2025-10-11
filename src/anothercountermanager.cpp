#include "countermanager.h"
#include "kkr_countergen/countergenerator.h"
#include <QMessageBox>
#include <QByteArray>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <openssl/err.h>

bool CounterManager::resources_initialized = false;

CounterManager::CounterManager(QObject *parent)
    : QObject(parent), m_data()
{
    if (!CounterManager::resources_initialized) {
        Q_INIT_RESOURCE(eruces);
        CounterManager::resources_initialized = true;
    }

    QFile pubkey(":/rsa/public");
    if (pubkey.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream reader(&pubkey);
        m_publicKeyPem = reader.readAll().trimmed();
        pubkey.close();
    } else {
        emit counterError("Couldn't read public key");
        return;
    }

    QFile privkey(":/rsa/private"); // Assumed resource; adjust as needed
    if (privkey.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream reader(&privkey);
        m_privateKeyPem = reader.readAll().trimmed();
        privkey.close();
    } else {
        emit counterError("Couldn't read private key");
        return;
    }

    initializeSettings();
    load();
    if (m_data.machineID.isEmpty()) {
        m_data.machineID = generateID();
        m_data.appID = "MyAppV1";
        m_data.version = 1;
        save();
    }
}

CounterManager::~CounterManager() {
    qDeleteAll(m_settingsList);
}

void CounterManager::initializeSettings() {
    QStringList paths = {
        "HKEY_CURRENT_USER\\System\\MyApp",
        QDir::homePath() + "/.config/myapp_settings1.ini",
        QCoreApplication::applicationDirPath() + "/settings2.ini",
        QDir::tempPath() + "/myapp_settings3.ini"
    };

    for (const QString& path : paths) {
        QSettings* settings = new QSettings(path, path.startsWith("HKEY_") ? QSettings::NativeFormat : QSettings::IniFormat, this);
        if (settings->isWritable()) {
            m_settingsList.append(settings);
        } else {
            qDebug() << "Cannot write to" << path;
            delete settings;
        }
    }
}

bool CounterManager::isValid() const {
    return m_data.isValid() && m_data.counterLeft > 0;
}

void CounterManager::decrement() {
    if (m_data.counterLeft > 0) {
        m_data.counterLeft--;
        m_data.usageCounter++;
        m_data.version++;
        save();
    }
}

bool CounterManager::addFromToken(const QString& token) {
    QByteArray decoded = QByteArray::fromBase64(token.toUtf8());
    QString decodedStr = QString::fromUtf8(decoded);
    QStringList parts = decodedStr.split(":");

    if (parts.size() != 2) {
        return false;
    }

    QString data = QByteArray::fromBase64(parts[0]);
    QString signatureBase64 = parts[1];

    QStringList dataParts = data.split(":");
    if (dataParts.size() != 3) {
        return false;
    }

    QString id = dataParts[0];
    QString dateStr = dataParts[1];
    int value = dataParts[2].toInt();

    if (id != m_data.machineID) {
        return false;
    }

    QDateTime tokenDate = QDateTime::fromString(dateStr, "yyyy-MM-dd");
    if (!tokenDate.isValid() || tokenDate <= m_data.lastRefill) {
        return false;
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.toUtf8().constData()), data.toUtf8().size(), hash);

    QByteArray signature = QByteArray::fromBase64(signatureBase64.toUtf8());
    BIO* bio = BIO_new_mem_buf(m_publicKeyPem.toUtf8().constData(), -1);
    RSA* rsaPubKey = PEM_read_bio_RSA_PUBKEY(bio, NULL, NULL, NULL);
    BIO_free(bio);
    if (!rsaPubKey) {
        return false;
    }

    int verifyStatus = RSA_verify(NID_sha256, hash, SHA256_DIGEST_LENGTH,
                                 reinterpret_cast<const unsigned char*>(signature.constData()),
                                 signature.size(), rsaPubKey);
    RSA_free(rsaPubKey);

    if (verifyStatus != 1) {
        return false;
    }

    m_data.counterLeft += value;
    m_data.lastRefill = tokenDate;
    m_data.usageCounter++;
    m_data.version++;
    save();
    return true;
}

QString CounterManager::getInstallationID() const {
    return m_data.machineID;
}

int CounterManager::getCounter() const {
    return m_data.counterLeft;
}

QByteArray CounterManager::encryptData(const QString& data) const {
    BIO* bio = BIO_new_mem_buf(m_publicKeyPem.toUtf8().constData(), -1);
    RSA* rsaPubKey = PEM_read_bio_RSA_PUBKEY(bio, NULL, NULL, NULL);
    BIO_free(bio);
    if (!rsaPubKey) {
        emit counterError("Failed to load public key for encryption");
        return QByteArray();
    }

    QByteArray input = data.toUtf8();
    int rsaLen = RSA_size(rsaPubKey);
    QByteArray encrypted(rsaLen, 0);

    int result = RSA_public_encrypt(input.size(), reinterpret_cast<const unsigned char*>(input.constData()),
                                   reinterpret_cast<unsigned char*>(encrypted.data()), rsaPubKey, RSA_PKCS1_PADDING);
    RSA_free(rsaPubKey);

    if (result == -1) {
        emit counterError(QString("Encryption failed: %1").arg(ERR_error_string(ERR_get_error(), nullptr)));
        return QByteArray();
    }

    return encrypted.left(result);
}

QString CounterManager::decryptData(const QByteArray& encrypted) const {
    BIO* bio = BIO_new_mem_buf(m_privateKeyPem.toUtf8().constData(), -1);
    RSA* rsaPrivKey = PEM_read_bio_RSAPrivateKey(bio, NULL, NULL, NULL);
    BIO_free(bio);
    if (!rsaPrivKey) {
        emit counterError("Failed to load private key for decryption");
        return QString();
    }

    int rsaLen = RSA_size(rsaPrivKey);
    QByteArray decrypted(rsaLen, 0);

    int result = RSA_private_decrypt(encrypted.size(), reinterpret_cast<const unsigned char*>(encrypted.constData()),
                                    reinterpret_cast<unsigned char*>(decrypted.data()), rsaPrivKey, RSA_PKCS1_PADDING);
    RSA_free(rsaPrivKey);

    if (result == -1) {
        emit counterError(QString("Decryption failed: %1").arg(ERR_error_string(ERR_get_error(), nullptr)));
        return QString();
    }

    return QString::fromUtf8(decrypted.left(result));
}

void CounterManager::load() {
    QMap<QString, QList<AppSettingsData>> groups;
    QStringList validKeys;

    for (QSettings* settings : m_settingsList) {
        QByteArray encrypted = settings->value("encryptedData").toByteArray();
        if (encrypted.isEmpty()) {
            qDebug() << "No encrypted data in" << settings->fileName();
            continue;
        }

        QString decrypted = decryptData(encrypted);
        if (decrypted.isEmpty()) {
            qDebug() << "Decryption failed for" << settings->fileName();
            continue;
        }

        AppSettingsData data = AppSettingsData::deserialize(decrypted);
        if (data.isValid()) {
            QString key = data.machineID + "|" + data.appID;
            groups[key].append(data);
            if (!validKeys.contains(key)) {
                validKeys.append(key);
            }
        } else {
            qDebug() << "Invalid data in" << settings->fileName();
        }
    }

    if (groups.isEmpty()) {
        qDebug() << "No valid settings, using defaults.";
        m_data = AppSettingsData();
        return;
    }

    int highestVersion = -1;
    QDateTime latestRefill;
    for (const QString& key : validKeys) {
        for (const auto& data : groups[key]) {
            if (data.version > highestVersion ||
                (data.version == highestVersion && data.lastRefill > latestRefill)) {
                m_data = data;
                highestVersion = data.version;
                latestRefill = data.lastRefill;
            }
        }
    }

    qDebug() << "Loaded settings with version" << m_data.version;
    m_data.print();
}

void CounterManager::save() {
    QString serialized = m_data.serialize();
    QByteArray encrypted = encryptData(serialized);
    if (encrypted.isEmpty()) {
        emit counterError("Failed to encrypt data for saving");
        return;
    }

    QString base64Encrypted = encrypted.toBase64();
    for (QSettings* settings : m_settingsList) {
        if (settings->isWritable()) {
            settings->setValue("encryptedData", base64Encrypted);
            settings->sync();
        } else {
            qDebug() << "Cannot write to" << settings->fileName();
        }
    }
}

QString CounterManager::generateID() {
#ifdef _WIN32
    DWORD serial = 0;
    if (GetVolumeInformationA("C:\\", NULL, 0, &serial, NULL, NULL, NULL, 0)) {
        return QString::number(serial, 16).toUpper();
    } else {
        qDebug() << "Failed to get volume serial for C:";
        return QUuid::createUuid().toString(QUuid::WithoutBraces).toUpper();
    }
#else
    return QUuid::createUuid().toString(QUuid::WithoutBraces).toUpper();
#endif
}