#include "btekcomm.h"
#include "cwinbtek.h"
#include <QIODevice>

btekcomm::btekcomm(QWidget* parent)
  : QMainWindow(parent)
{
  ui.setupUi(this);
  CComm* mCComm = new CComm(this);
  mCComm->open(QIODevice::ReadWrite);
  char tmpb[4] = { 0 };
  tmpb[0] = 0x83;
  tmpb[1] = 0xc8;
  tmpb[2] = 0x23;
  tmpb[3] = 0x6f;
  mCComm->write(tmpb, sizeof(tmpb));
}
