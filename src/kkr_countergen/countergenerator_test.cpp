#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include "countergenerator.h"
#include <QDebug>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    Q_INIT_RESOURCE(dev_eruces);
    // Test parameters
    QString clientId = "testClient123";
    int value = 42;
    QString tanggal = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    QString rawToken = QString("%1:%2:%3").arg(clientId, tanggal, QString::number(value));
    
    // Test 1: Generate a token
    qDebug() << "Generating token for clientId:" << clientId << "and value:" << value;
    QByteArray token = generateToken(clientId, value);
    if (token.isEmpty()) {
        qDebug() << "Error: Token generation failed!";
        return 1;
    }
    qDebug() << "Generated token:" << token.toBase64();

    // Test 2: Validate the token
    bool ok;
    QString tokenData = validateToken(token, &ok);
    if (ok) {
        qDebug() << "Token validation successful!";
        if (tokenData == rawToken) {
            qDebug() << "Success: Client ID matches!";
            qDebug() << "Token Data" << tokenData;
        } else {
            qDebug() << "Error: Client ID mismatch!";
            return 1;
        }
    } else {
        qDebug() << "Error: Token validation failed!";
        return 1;
    }

    return 0;
}