#include "notifier.h"

Notifier::Notifier()
: _cancel(false), QObject() {}

void Notifier::emitCurrent(int c) {
  emit currentChanged(c);
}

void Notifier::emitMaximum(int mx) {
  emit maximumChanged(mx);
}

void Notifier::emitMinimum(int mn) {
  emit minimumChanged(mn);
}

void Notifier::emitMessage(const QString &msg) {
  emit messageChanged(msg);
}

void Notifier::cancelRequested() {
  _cancel = true;
}