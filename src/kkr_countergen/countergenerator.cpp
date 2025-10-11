#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <QDateTime>
#include <QFile>
#include <QDebug>

#include "countergenerator.h"
// /* ===================== BACKUP =======================
bool verifySignature(const QByteArray& dataPart, const QByteArray& signature) {
  
  // Load public key from resource
  QFile publicFile(":/rsa/public");
  if (!publicFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
      qDebug() << "Failed to open public key file";
      return false;
  }
  
  QByteArray publicKeyData = publicFile.readAll();
  publicFile.close();
    
  // Convert to BIO for OpenSSL
  BIO* bio = BIO_new_mem_buf(publicKeyData.constData(), publicKeyData.size());
  if (!bio) {
      qDebug() << "Failed to create BIO for public key";
      return false;
  }
    
  EVP_PKEY* pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
  BIO_free(bio);
  if (!pkey) {
      qDebug() << "Failed to load public key:" << ERR_error_string(ERR_get_error(), nullptr);
      return false;
  }
  
  if (!pkey) {
      qDebug() << "Failed to load public key:" << ERR_error_string(ERR_get_error(), nullptr);
      return false;
  }
  qDebug() << "Public key loaded OK (type:" << EVP_PKEY_id(pkey) << ", bits:" << EVP_PKEY_bits(pkey) << ")";  // NEW: Key info
  qDebug() << "Data len:" << dataPart.size() << "Sig len:" << signature.size();  // NEW: Sizes for match
  
  // Verify signature with EVP (feed original data; OpenSSL handles SHA256 internally)
  EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
  if (!md_ctx) {
      qDebug() << "Failed to create EVP_MD_CTX";
      EVP_PKEY_free(pkey);
      return false;
  }
    
  // Initialize verification
  if (EVP_DigestVerifyInit(md_ctx, nullptr, EVP_sha256(), nullptr, pkey) <= 0) {
      qDebug() << "Failed to initialize verification:" << ERR_error_string(ERR_get_error(), nullptr);
      EVP_MD_CTX_free(md_ctx);
      EVP_PKEY_free(pkey);
      return false;
  }
  
  qDebug() << "Verification init OK";  // NEW
    
  // Update verification with original data
  if (EVP_DigestVerifyUpdate(md_ctx, reinterpret_cast<const unsigned char*>(dataPart.constData()), dataPart.size()) <= 0) {
      qDebug() << "Failed to update verification:" << ERR_error_string(ERR_get_error(), nullptr);
      EVP_MD_CTX_free(md_ctx);
      EVP_PKEY_free(pkey);
      return false;
  }
  
  qDebug() << "Update OK";  // NEW
  
  // Verify signature
  int verifyResult = EVP_DigestVerifyFinal(md_ctx, reinterpret_cast<const unsigned char*>(signature.constData()), signature.size());
  qDebug() << "Final verify result:" << verifyResult << "(1=good, 0=bad, <0=error)";  // NEW: Exact code
  EVP_MD_CTX_free(md_ctx);
  EVP_PKEY_free(pkey);
    
  return verifyResult == 1;
}
// */

/* ===================== BACKUP =======================
QString validateToken(const QByteArray& arr, bool* ok) {
    // Initialize ok flag
    *ok = false;
    
    // Decompress and split token
    QByteArray uncompressed = qUncompress(arr);
    if (uncompressed.isEmpty()) {
        qDebug() << "Failed to decompress token";
        return QString();
    }
    
    QStringList base64_pair = QString(uncompressed).split(':');
    if (base64_pair.size() != 2) {
        qDebug() << "Invalid token format" << base64_pair;
        qDebug() << "base64_pair Count" << base64_pair.size();
        return QString();
    }
    
    QByteArray retByte = QByteArray::fromBase64(base64_pair[0].toUtf8());
    QByteArray retSign = QByteArray::fromBase64(base64_pair[1].toUtf8());
    
    int trusted = verifySignature(retByte, retSign);
    if(!trusted) {
      qDebug() << "Signature Verification Failed";
      return QString();
    }
    
    // Decompress the data part
    QByteArray data = qUncompress(retByte);
    if (data.isEmpty()) {
        qDebug() << "Failed to decompress data part";
        return QString();
    }
    
    // Extract and validate payload (must be exactly 3 parts)
    QStringList tokenData = QString(data).split("::");
    if (tokenData.size() != 3) {
        qDebug() << "Invalid token data format: expected 3 parts, got" << tokenData.size();
        return QString();
    }
    
    // Optional: Basic date validation (e.g., reject if older than 30 days)
    // QDate tokenDate = QDate::fromString(tokenData[1], "yyyy-MM-dd");
    // if (!tokenDate.isValid() || tokenDate.daysTo(QDate::currentDate()) > 30) {
    //     qDebug() << "Token expired or invalid date";
    //     return QString();
    // }

    *ok = true;
    return data; // Returns "clientId::date::value"
} */

QString validateToken(const QByteArray& arr, bool* ok) {
    // Initialize ok flag
    *ok = false;
    
    // Decompress token (binary)
    QByteArray uncompressed = qUncompress(arr);
    if (uncompressed.isEmpty()) {
        qDebug() << "Failed to decompress token";
        return QString();
    }
    
    // Find the exact position of the single ':' separator (byte-level, no QString mangle)
    int colonPos = uncompressed.indexOf(':');
    if (colonPos == -1) {
        qDebug() << "No ':' separator found in decompressed token";
        return QString();
    }
    if (uncompressed.indexOf(':', colonPos + 1) != -1) {  // Check for extras
        qDebug() << "Multiple ':' separators found—token corrupted";
        return QString();
    }
    
    // Split into data_b64 and sig_b64 (raw bytes)
    QByteArray dataB64 = uncompressed.left(colonPos);
    QByteArray sigB64 = uncompressed.mid(colonPos + 1);
    
    qDebug() << "base64_pair Count" << (dataB64.isEmpty() || sigB64.isEmpty() ? 0 : 2);  // Your debug log (remove later)
    
    QByteArray retByte = QByteArray::fromBase64(dataB64);
    QByteArray retSign = QByteArray::fromBase64(sigB64);
    if (retByte.isEmpty() || retSign.isEmpty()) {
        qDebug() << "Base64 decode failed: data len" << retByte.size() << "sig len" << retSign.size();
        return QString();
    }
    
    int trusted = verifySignature(retByte, retSign);
    if (!trusted) {
      qDebug() << "Signature Verification Failed";
      return QString();
    }
    
    // Decompress the data part
    QByteArray data = qUncompress(retByte);
    if (data.isEmpty()) {
        qDebug() << "Failed to decompress data part";
        return QString();
    }
    
    // Extract and validate payload (must be exactly 3 parts)
    QStringList tokenData = QString(data).split("::");
    if (tokenData.size() != 3) {
        qDebug() << "Invalid token data format: expected 3 parts, got" << tokenData.size();
        return QString();
    }
    
    // Optional: Basic date validation (e.g., reject if older than 30 days)
    // QDateTime tokenDate = QDateTime::fromString(tokenData[1], "yyyy-MM-ddTHH:mm:ss");
    // if (!tokenDate.isValid() || tokenDate < QDateTime::currentDateTime().addDays(-30)) {
    //     qDebug() << "Token expired or invalid date";
    //     return QString();
    // }

    *ok = true;
    return data; // Returns "clientId::date::value"
}

QByteArray createSignature(const QByteArray& data){
  
    // Guard empty input
    if (data.isEmpty()) {
        qDebug() << "Empty data provided for signing";
        return QByteArray();
    }
    
    // Load private key from resource
    QFile privateFile(":/rsa/private");
    if (!privateFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open private key file";
        return QByteArray();
    }

    QByteArray privateKeyData = privateFile.readAll();
    privateFile.close();

    // Convert to BIO for OpenSSL
    BIO* bio = BIO_new_mem_buf(privateKeyData.constData(), privateKeyData.size());
    if (!bio) {
        qDebug() << "Failed to create BIO for private key";
        return QByteArray();
    }

    EVP_PKEY* pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    if (!pkey) {
        qDebug() << "Failed to load private key:" << ERR_error_string(ERR_get_error(), nullptr);
        return QByteArray();
    }

    // Sign with EVP (feed original data; OpenSSL handles SHA256 internally)
    EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
    if (!md_ctx) {
        qDebug() << "Failed to create EVP_MD_CTX";
        EVP_PKEY_free(pkey);
        return QByteArray();
    }
    
    // Init Digest
    if (EVP_DigestSignInit(md_ctx, nullptr, EVP_sha256(), nullptr, pkey) <= 0) {
        qDebug() << "Failed to initialize signing:" << ERR_error_string(ERR_get_error(), nullptr);
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
        return QByteArray();
    }

    // Update Digest with original data
    if (EVP_DigestSignUpdate(md_ctx, reinterpret_cast<const unsigned char*>(data.constData()), data.size()) <= 0) {
        qDebug() << "Failed to update signing:" << ERR_error_string(ERR_get_error(), nullptr);
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
        return QByteArray();
    }

    size_t sigLen;
    if (EVP_DigestSignFinal(md_ctx, nullptr, &sigLen) <= 0) {
        qDebug() << "Failed to get signature length:" << ERR_error_string(ERR_get_error(), nullptr);
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
        return QByteArray();
    }

    unsigned char* signature = new unsigned char[sigLen];
    if (EVP_DigestSignFinal(md_ctx, signature, &sigLen) <= 0) {
        qDebug() << "Failed to finalize signing:" << ERR_error_string(ERR_get_error(), nullptr);
        delete[] signature;
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
        return QByteArray();
    }

    EVP_MD_CTX_free(md_ctx);
    EVP_PKEY_free(pkey);

    // Encode signature to base64
    QByteArray ret(reinterpret_cast<char*>(signature), sigLen);
    delete[] signature;
    
    return ret.toBase64();  // Base64-encode here for consistency (was missing in original)
}

QByteArray generateToken(const QString& clientId, int value) {
    // Guard empty clientId
    if (clientId.isEmpty()) {
        qDebug() << "Empty clientId provided";
        return QByteArray();
    }
    
    // Prepare data
    QString cDate = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QStringList tokenData {clientId, cDate, QString::number(value)};
    QByteArray byteData = qCompress(tokenData.join("::").toUtf8(), 9);
    
    QByteArray sigBytes = createSignature(byteData);
    if (sigBytes.isEmpty()) {
        qDebug() << "Failed to create signature";
        return QByteArray();
    }
    
    qDebug() << "byteData" << byteData.toBase64();  // Debug: base64 for readability
    qDebug() << "sigBytes" << sigBytes;
    
    // Join data (base64) + ':' + signature (already base64'd)
    QByteArray token = qCompress(byteData.toBase64() + ':' + sigBytes, 9);
    qDebug() << "Token" << token.toBase64();  // Debug: base64 for readability
    return token;
}