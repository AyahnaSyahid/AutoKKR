#ifndef POLLEDSETTINGS_H
#define POLLEDSETTINGS_H

#include <QSettings>

struct CounterApplicationData {
  QString MachineID; 
  QString AppID;     
  QString LastRefillTime;
  int UsageCounter;
  int CounterLeft;
};

class PolledSettings {
  
};


#endif // POOLEDSETTINGS_H