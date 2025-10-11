#ifndef POLLEDSETTINGS_H
#define POLLEDSETTINGS_H

#include <QSettings>
#include <QMap>

struct CounterApplicationData {
  QString MachineID;
  QString AppID;     
  QString LastRefillTime;
  int UsageCounter;
  int CounterLeft;
  qreal trustLevel;
  struct Refs {
    int usage, left;
    QString last;
    Refs(const CounterApplicationData &ca) 
      : usage(ca.UsageCounter), left(ca.CounterLeft), last(ca.LastRefillTime) {};
    bool operator==(const Refs &rhs) {
      return usage == rhs.usage && left == rhs.left && last == rhs.last;
    };
  };
  
  Refs asRefs() const {
    return Refs(*this);
  };
};

class QAction;
class PolledSettings {

  public:
    explicit PolledSettings(const QString &appId);
    ~PolledSettings();
    CounterApplicationData appData() const { return m_cached; };
    inline const qreal &trustLevel() const { return m_trust; };
    void incrementUsage(int incr);
    void refillCounter(int value); // automatically set lastRefill
    // void setLastRefill(const QString& );
    void analize();
    const QString &machineId() const { return m_mId; }
    QString installID() const;
    inline QAction* clearSettingsAction() const { return actionClearSettings; }

  private:
    void writeBack();
    void clearAllSettings();
    QMap<QString, QSettings::Format> m_config;
    CounterApplicationData m_cached;
    qreal m_trust;
    QString m_mId;
    QString m_appId;
    QAction *actionClearSettings;
};

#endif // POLLEDSETTINGS_H