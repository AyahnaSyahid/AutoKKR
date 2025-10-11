#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <QDateTime>
#include <QFile>
#include <QDebug>

#include "countergenerator.h"

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
    
    // Compute SHA256 hash of data
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(dataPart.constData()), dataPart.size(), hash);
    
    // Verify signature with EVP
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
    
    // Update verification
    if (EVP_DigestVerifyUpdate(md_ctx, hash, SHA256_DIGEST_LENGTH) <= 0) {
        qDebug() << "Failed to update verification:" << ERR_error_string(ERR_get_error(), nullptr);
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
        return false;
    }
    
    // Verify signature
    int verifyResult = EVP_DigestVerifyFinal(md_ctx, reinterpret_cast<const unsigned char*>(signature.constData()), signature.size());
    EVP_MD_CTX_free(md_ctx);
    EVP_PKEY_free(pkey);
    
    return verifyResult == 1;
}

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
    
    // If verification successful, extract clientId
    QStringList tokenData = QString(data).split("::");
    if (tokenData.size() < 1) {
        qDebug() << "Invalid token data format";
        return QString();
    }

    if(ok) *ok = true;
    return data; // Return clientId:date:value
}

QByteArray createSignature(const QByteArray& data){
  
    // Load private key from resource
    QByteArray byteData(data);
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

    // Compute SHA256 hash of data
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(byteData.constData()), byteData.size(), hash);

    // Sign with EVP (SHA256, PKCS1 padding)
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

    // Update Digest
    if (EVP_DigestSignUpdate(md_ctx, hash, SHA256_DIGEST_LENGTH) <= 0) {
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
    
    return ret;
}

QByteArray generateToken(const QString& clientId, int value) {

    // Prepare data
    QString cDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    QStringList tokenData {clientId, cDate, QString::number(value)};
    QByteArray byteData = qCompress(tokenData.join("::").toUtf8(), 9);
    
    QByteArray sigBytes = createSignature(byteData);
    qDebug() << "byteData " << byteData;
    qDebug() << "sigBytes " << sigBytes;
    
    QByteArray token = qCompress(byteData.toBase64() + ':' + sigBytes.toBase64(), 9);
    // Token is Compressed
    qDebug() << "Token " << token;
    return token;
}

