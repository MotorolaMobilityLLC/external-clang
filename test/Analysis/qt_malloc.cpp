// RUN: %clang_cc1 -analyze -analyzer-checker=core,alpha.deadcode.UnreachableCode,alpha.core.CastSize,unix.Malloc,cplusplus -analyzer-store=region -verify %s
// expected-no-diagnostics
#include "Inputs/qt-simulator.h"

void send(QObject *obj)
{
  QEvent *e1 = new QEvent(QEvent::None);
  static_cast<QApplication *>(QCoreApplication::instance())->postEvent(obj, e1);
  QEvent *e2 = new QEvent(QEvent::None);
  QCoreApplication::instance()->postEvent(obj, e2);
  QEvent *e3 = new QEvent(QEvent::None);
  QCoreApplication::postEvent(obj, e3);
  QEvent *e4 = new QEvent(QEvent::None);
  QApplication::postEvent(obj, e4);
}
