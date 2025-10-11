#include "countermanager.h"
#include <QByteArray>
#include <QDebug>
#include <QBase64>  // Wait, Qt has QByteArray::fromBase64

CounterManager::CounterManager()
    : m_counter(0),
      m_settings(new QSettings("MyAppOrg", "MyAppName"))
{
    // Hardcode public key PEM (replace with your own generated public key!)
    m_publicKeyPem = R"(
-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA3Tz2mr7SZiAMfQyuvBjM9Oi0W4cOBZ8+TB/L8sNIE4uc+dybzwbIr3RbgJmn/IJzSKfNY/Hv0yM7GU9/W5w6Xp6VlyoygseuNx+eoS4vJMn+lM4LNxPkPw0QX+0KZs+TFGPCe3vYp6z5YSci+Cr16D7lA4J0jfkdyKx9yWYp5zYkFnvlC8hGmd4Ww/u97k7PfTLcmfZffpbjOSQVVbWVLp7zrd0sQEN053rUihlGWo5v/m5U6SGr8pR/0kmP2UlJNVusre4jS8hVo1p4S3x8y6Ma4H8jP/ +kBSEmd1jA7A+3z0Pmpo484P/RykgVTiFVEpNCgHbB/6rX3zPxUMAA
-----END PUBLIC KEY-----
)";

    // Trim whitespace
    m_publicKeyPem = m_publicKeyPem.trimmed();

    load();
    if (m_localID.isEmpty()) {
        m_localID = generateID();
        save();
    }
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

    QString data = parts[0];
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