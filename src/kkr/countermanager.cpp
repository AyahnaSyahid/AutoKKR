#include "countermanager.h"
#include "kkr_countergen/countergenerator.h"
#include <QMessageBox>
#include <QByteArray>
#include <QTextStream>
#include <QFile>
#include <QFile>
#include <QDebug>

bool CounterManager::resources_initialized = false;

CounterManager::CounterManager()
    : m_counter(0),
      m_settings(new QSettings(this))
{
    if(!CounterManager::resources_initialized) {
      Q_INIT_RESOURCES(eruces);
    }

    QFile pubkey(":/rsa/public");
    if(pubkey.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QTextStream reader(&pubkey);
      m_publicKeyPem = reader.readAll();
      m_publicKeyPem = m_publicKeyPem.trimmed();
      pubkey.close();
      load();
      if (m_localID.isEmpty()) {
          m_localID = generateID();
          save();
      }
    }
    emit counterError("Couldn't read public key");
}

CounterManager::~CounterManager() {
    delete m_settings;
}

bool CounterManager::isValid() const {
    return m_counter > 0;
}

void CounterManager::decrement() {
    if (m_counter > 0) {
        m_counter--;
        save();
    }
}

bool CounterManager::addFromToken(const QString& token) {
    
    // Decode base64
    QByteArray decoded = QByteArray::fromBase64(token.toUtf8());
    QString decodedStr = QString::fromUtf8(decoded);
    QStringList parts = decodedStr.split(":");

    if (parts.size() != 2) {
        return false;  // Invalid format: data:signature
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

    // Verify ID matches local
    if (id != m_localID) {
        return false;
    }

    // Verify date
    QDate tokenDate = QDate::fromString(dateStr, "yyyy-MM-dd");
    QDate lastFillDate = QDate::fromString(m_lastDate, "yyyy-MM-dd");
    if (!tokenDate.isValid() || (tokenDate <= lastFillDate)) {
        return false;  // Invalid or outdated date
    }

    // Prepare SHA256 hash of data
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.toUtf8().constData()), data.toUtf8().size(), hash);

    // Decode signature
    QByteArray signature = QByteArray::fromBase64(signatureBase64.toUtf8());

    // Load public key
    BIO* bio = BIO_new_mem_buf(m_publicKeyPem.toUtf8().constData(), -1);
    RSA* rsaPubKey = PEM_read_bio_RSA_PUBKEY(bio, NULL, NULL, NULL);
    BIO_free(bio);
    if (!rsaPubKey) {
        return false;  // Failed to load public key
    }

    // Verify signature (RSA_PKCS1_PADDING)
    int verifyStatus = RSA_verify(NID_sha256, hash, SHA256_DIGEST_LENGTH, reinterpret_cast<const unsigned char*>(signature.constData()), signature.size(), rsaPubKey);
    RSA_free(rsaPubKey);

    if (verifyStatus != 1) {
        return false;  // Signature verification failed
    }

    // All valid: add value and update last date
    m_counter += value;
    m_lastDate = dateStr;
    save();
    return true;
}

QString CounterManager::getInstallationID() const {
    return m_localID;
}

int CounterManager::getCounter() const {
    return m_counter;
}

void CounterManager::load() {
    m_counter = m_settings->value("counter", 0).toInt();
    m_lastDate = m_settings->value("lastDate", "").toString();
    m_localID = m_settings->value("localID", "").toString();
}

void CounterManager::save() {
    m_settings->setValue("counter", m_counter);
    m_settings->setValue("lastDate", m_lastDate);
    m_settings->setValue("localID", m_localID);
    m_settings->sync();
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