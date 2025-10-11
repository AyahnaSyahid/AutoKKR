#include <QByteArray>
#include <QString>

// Signature Creation
QByteArray createSignature(const QByteArray& data);

// Data + Signature Veification
bool verifySignature(const QByteArray& dataPart, const QByteArray& signature);

// Return (QString) "client:date:val" if ok == true else QString()
QString validateToken(const QByteArray &ba, bool *ok = nullptr);

/* Generate Token
  ---- FORMAT
  token ( 
    qCompress (
      base64-of (
        byteData ( 
          qCompress (
            tokenData ( "clientId:cDate:value" )
          )
        )
      ), 
      ':',
      base64-of (
        sigBytes (
          signature-of ( byteData )
        )
      )
    )
  )
************************************************************/
QByteArray generateToken(const QString& clientId, int value);
