#include "colorconverter.h"
#include <QResource>
#include <QFile>
#include <QByteArray>
#include <QtDebug>

cmsHPROFILE ColorManagement::cmykProfile = nullptr ,
            ColorManagement::proofProfile = nullptr , 
            ColorManagement::rgbProfile = nullptr ;
cmsHTRANSFORM ColorManagement::proofTransform = nullptr,
              ColorManagement::rgbTransform = nullptr ;

cmsHPROFILE ColorManagement::loadIccFromResource(const QString& resourcePath) {
    // Step 1: Access the resource
    QFile icc(resourcePath);  // e.g., ":/profiles/MyProfile.icc"
    if(!icc.open(QIODevice::ReadOnly)) {
        qWarning() << "Tidak dapat membaca :" << resourcePath;
        return nullptr;
    }

    // Step 2: Read raw bytes (handles compression automatically)
    QByteArray data = icc.readAll();
    qint64 size = icc.size();
    if (size <= 0) {
        qWarning() << "Empty resource:" << resourcePath;
        return nullptr;
    }

    // Step 3: Load into LCMS2 from memory
    // cmsOpenProfileFromMem takes const void*, so cast safely
    cmsHPROFILE profile = cmsOpenProfileFromMem(data.data() , static_cast<cmsUInt32Number>(size));
    if (!profile) {
        qWarning() << "Failed to open profile from memory";
    }

    return profile;
}

void ColorManagement::init() {
  cmykProfile = loadIccFromResource(":/profiles/CMYK");
  proofProfile = loadIccFromResource(":/profiles/SRGB");
  rgbProfile = cmsCreate_sRGBProfile();
  if(!cmykProfile || !proofProfile || !rgbProfile) {
    qWarning() << "Tidak dapat membaca profile";
    return;
  }
  proofTransform = cmsCreateProofingTransform(
            cmykProfile, TYPE_CMYK_8, // Source: 8-bit CMYK
            rgbProfile, TYPE_RGB_8,   // Destination: 8-bit RGB
            proofProfile,             // Proofing profile
            INTENT_PERCEPTUAL,        // Source->Proof intent
            INTENT_PERCEPTUAL,        // Proof->Destination intent
            cmsFLAGS_SOFTPROOFING     // Enable soft proofing
        );
  
  rgbTransform = cmsCreateTransform(
    proofProfile, TYPE_RGB_8,
    rgbProfile, TYPE_RGB_8,
    INTENT_PERCEPTUAL, 0);
  
  cmsCloseProfile(cmykProfile);
  cmsCloseProfile(rgbProfile);
  cmsCloseProfile(proofProfile);
  
  if (!proofTransform || !rgbTransform) {
      qDebug() << "Error: Could not create proofing transform.";
  }
}

QColor ColorManagement::proof_of(const QColor& cl) {
  if(cl.spec() == QColor::Rgb) {
    if(!rgbTransform) {
      return cl;
    }
    unsigned char input[3], output[3];
    cmsDoTransform(rgbTransform, input, output, 1);
    return QColor(output[0], output[2], output[2]);
  } else if (cl.spec() == QColor::Cmyk) {
    if(!proofTransform) {
      return cl;
    }
  unsigned char input_cmyk[4] = {
            static_cast<unsigned char>(cl.cyan()),
            static_cast<unsigned char>(cl.magenta()),
            static_cast<unsigned char>(cl.yellow()),
            static_cast<unsigned char>(cl.black())
        };
  // Convert CMYK to RGB
  unsigned char output_rgb[3] = {0, 0, 0};
  cmsDoTransform(proofTransform, input_cmyk, output_rgb, 1);
  return QColor(output_rgb[0], output_rgb[1], output_rgb[2]);
  }
  return cl;
}