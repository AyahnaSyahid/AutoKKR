#include "polledsettings.h"
#ifdef _WIN32
  #include <windows.h>
#endif
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QApplication>
#include <QtMinMax>
#include <QUuid>
#include <QAction>
#include <QDebug>

PolledSettings::PolledSettings(const QString &appId)
  : actionClearSettings(new QAction), m_mId {}, m_appId(appId), m_cached {}, m_config {} {
    m_mId = installID();
    m_cached.MachineID = m_mId;
    QDir appPath(qApp->applicationDirPath());
    m_config[QString("HKEY_CURRENT_USER\\SOFTWARE\\Corel\\CorelDRAW\\X26\\%1\\Draw").arg(m_mId)] = QSettings::NativeFormat;
    m_config[QString("HKEY_CURRENT_USER\\SOFTWARE\\MPC-HC\\%1\\License").arg(m_mId)] = QSettings::NativeFormat;
    m_config[QString("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\DirectX20\\%1\\Version").arg(m_mId)] = QSettings::NativeFormat;
    m_config[QString("HKEY_CURRENT_USER\\SOFTWARE\\BlackCircle\\%1").arg(m_mId)] = QSettings::NativeFormat;
    m_config[QString("HKEY_CURRENT_USER\\SOFTWARE\\Realtek\\%1\\Drivers").arg(m_mId)]= QSettings::NativeFormat;
    m_config[QString("HKEY_CURRENT_USER\\SOFTWARE\\Adobe\\Updater\\%1\\Updates").arg(m_mId)] = QSettings::NativeFormat;
    m_config[appPath.filePath("data/" + QString("cache/%1/m.conf").arg(m_mId))] = QSettings::IniFormat;
    m_config[QDir::home().filePath("AppData/Local/Programs/BlackCircle/" + QString("%1.xml").arg(m_mId))] = QSettings::IniFormat;
    m_config[QDir::home().filePath("AppData/LocalLow/Programs/.dummy/" + QString(".%1/jvm.jar").arg(m_mId))] = QSettings::IniFormat;
    m_config[QDir::home().filePath("AppData/Roaming/Programs/Corel/X26/" + QString("%1/runtime.log").arg(m_mId))] = QSettings::IniFormat;
    m_config[QDir::home().filePath("AppData/Local/vscode/crack/" + QString("%1/adv.1").arg(m_mId))] = QSettings::IniFormat;
    
    analize();
    
    actionClearSettings->connect(actionClearSettings, &QAction::triggered, [=](){
      clearAllSettings();
    });
}

PolledSettings::~PolledSettings(){
  delete actionClearSettings;
}

void PolledSettings::analize() {
  QList<QSettings*> ss;
  for(auto conf = m_config.cbegin(), end = m_config.cend(); conf != end; ++conf ) {
    ss << new QSettings(conf.key(), conf.value());
  }
  QMap<int, int> pollUsageCounter; // usage, refer times
  QMap<int, int> pollCounterLeft;  // counterLetf, refer times
  QMap<QString, int> pollLastRefill;  // refillDate, refer times
  
  for(auto s : ss) {
    auto usage = s->value(m_appId + "/usage").toInt();
    auto left  = s->value(m_appId + "/left").toInt();
    auto last  = s->value(m_appId + "/last").toString();
    
    if(pollUsageCounter.contains(usage)) {
      pollUsageCounter[usage] += 1;
    } else {
      pollUsageCounter[usage] = 1;
    }
    
    if(pollCounterLeft.contains(left)) {
      pollCounterLeft[left] += 1;
    } else {
      pollCounterLeft[left] = 1;
    }
    
    if(pollLastRefill.contains(last)) {
      pollLastRefill[last] += 1;
    } else {
      pollLastRefill[last] = 1;
    }
  }
  int maxUsageRef = 0, maxLeftRef = 0, maxRefillRef = 0;
  for(auto poll = pollUsageCounter.cbegin(), end = pollUsageCounter.cend();
          poll != end; ++poll) {
    maxUsageRef = maxUsageRef < poll.value() ? poll.value() : maxUsageRef;
  }
  for(auto poll = pollCounterLeft.cbegin(), end = pollCounterLeft.cend();
          poll != end; ++poll) {
    maxLeftRef = maxLeftRef < poll.value() ? poll.value() : maxLeftRef;
  }
  for(auto poll = pollLastRefill.cbegin(), end = pollLastRefill.cend();
          poll != end; ++poll) {
    maxRefillRef = maxRefillRef < poll.value() ? poll.value() : maxRefillRef;
  }
  
  int winnerUsage, winnerLeft;
  QString winnerDate = "";
  
  winnerUsage = pollUsageCounter.keys(maxUsageRef)[0];
  winnerLeft = pollCounterLeft.keys(maxLeftRef)[0];
  winnerDate = pollLastRefill.keys(maxRefillRef)[0];
  
  qreal trusts = 1.0;
  int blank = 0, differ = 0, okay = 0;
  for(auto s : ss) {
    auto fn = s->fileName();
    if(s->contains(m_appId + "/usage")) {
      auto usage = s->value(m_appId + "/usage").toInt();
      differ += usage == winnerUsage ? 0 : 1;
      okay += usage == winnerUsage ? 1 : 0;
    } else {
      blank += 1;
    }
    if(s->contains(m_appId + "/left")) {
      auto left = s->value(m_appId + "/left").toInt();
      differ += left == winnerLeft ? 0 : 1;
      okay += left == winnerUsage ? 1 : 0;
    } else {
      blank += 1;
    }
    if(s->contains(m_appId + "/last")) {
      auto last = s->value(m_appId + "/last").toString();
      differ += last == winnerDate ? 0 : 1;
      okay += last == winnerDate ? 1 : 0;
    } else {
      blank += 1;
    }
  }

  m_cached.LastRefillTime = winnerDate;
  m_cached.UsageCounter = winnerUsage;
  m_cached.CounterLeft = winnerLeft;
  
  switch (blank) {
    case 33:
      m_cached.trustLevel = 1.0;
      break;
    default: {
      int bp = blank * 3;
      bp += differ;
      m_cached.trustLevel = 1.0 - (bp / 33.0);
      if(okay == 33) {
        m_cached.trustLevel = 1.0;
      }
    }
  }

  if(m_cached.LastRefillTime == "") {
    // LastRefillTime not set
    m_cached.LastRefillTime = "2025-01-01 01:13:25";
  } else {
    QDateTime rec = QDateTime::fromString(m_cached.LastRefillTime, Qt::ISODate);
    if(!rec.isValid()) {
      m_cached.trustLevel = 0.0;
    }
  }
  m_trust = m_cached.trustLevel;
  qDeleteAll(ss.begin(), ss.end());
  ss.clear();
}

void PolledSettings::incrementUsage(int inc) {
  m_cached.UsageCounter += inc;
  m_cached.CounterLeft -= inc;
  writeBack();
}

void PolledSettings::refillCounter(int ref) {
  m_cached.CounterLeft += ref;
  m_cached.LastRefillTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
  writeBack();
}

void PolledSettings::writeBack() {
  for(auto conf = m_config.cbegin(), end = m_config.cend(); conf != end; ++conf ) {
    QSettings s(conf.key(), conf.value());
    if(!s.isWritable()) {
      qDebug() << "RO Detected " << s.fileName();
    } else {
      s.setValue(m_appId + "/usage", m_cached.UsageCounter);
      s.setValue(m_appId + "/left", m_cached.CounterLeft);
      s.setValue(m_appId + "/last", m_cached.LastRefillTime);
    }
  }
}

void PolledSettings::clearAllSettings() {
  struct sst {
    QString path, key;
    QSettings::Format ff;
  };
  QList<sst> trs; // To Be Removed Settings
  trs << sst {"HKEY_CURRENT_USER\\SOFTWARE\\Corel\\CorelDRAW", "X26", QSettings::NativeFormat};
  trs << sst {"HKEY_CURRENT_USER\\SOFTWARE\\MPC-HC", m_mId, QSettings::NativeFormat};
  trs << sst {"HKEY_CURRENT_USER\\SOFTWARE\\DirectX20", m_mId, QSettings::NativeFormat};
  trs << sst {"HKEY_CURRENT_USER\\SOFTWARE", "BlackCircle", QSettings::NativeFormat};
  trs << sst {"HKEY_CURRENT_USER\\SOFTWARE\\Realtek", m_mId, QSettings::NativeFormat};
  trs << sst {"HKEY_CURRENT_USER\\SOFTWARE\\Updater", m_mId, QSettings::NativeFormat};
}

QString PolledSettings::installID () const {
#ifdef _WIN32  
  TCHAR volumeName[MAX_PATH + 1] = { 0 };
  if (GetVolumeNameForVolumeMountPoint(TEXT("C:\\"), volumeName, MAX_PATH + 1)) {
      QString volName = QString::fromWCharArray(volumeName);
      auto x1 = volName.indexOf('{') + 1, x2 = volName.indexOf('}');
      volName = volName.mid(x1, x2 - x1).toUpper();
      // qDebug() << QString("GUID adalah %2").arg(volName);
      return volName;
  }
  // qDebug() << QString("Gagal mendapatkan GUID. Kode kesalahan: %d\n").arg(QString::number(GetLastError()));
  return QUuid::createUuid().toString(QUuid::WithoutBraces).toUpper();
#else
  return QUuid::createUuid().toString(QUuid::WithoutBraces).toUpper();
#endif
}