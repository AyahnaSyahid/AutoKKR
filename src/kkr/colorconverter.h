#ifndef COLORCONVERTER_H
#define COLORCONVERTER_H

#include <QColor>
#include <lcms2.h>

namespace ColorManagement {
  extern cmsHPROFILE cmykProfile;
  extern cmsHPROFILE proofProfile;
  extern cmsHPROFILE rgbProfile;
  extern cmsHTRANSFORM proofTransform;
  extern cmsHTRANSFORM rgbTransform;
  
  void init();
  cmsHPROFILE loadIccFromResource(const QString& resourcePath);
  QColor proof_of(const QColor& color);

};

#endif