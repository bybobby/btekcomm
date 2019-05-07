
#ifndef __CWINBRIDGETEK_H__

#define __CWINBRIDGETEK_H__

#ifndef COMM_BUFFERSIZE
  #define COMM_BUFFERSIZE 32768
#endif

#include <QDeadlineTimer>
#include <QtCore/private/qiodevice_p.h>

#define FTD2XX_STATIC 1

#include "ftd2xx.h"

#include <qt_windows.h>
#include <limits.h>

QT_BEGIN_NAMESPACE

class QWinOverlappedIoNotifier;
class QTimer;
class QSocketNotifier;

class CCommPrivate : public QIODevicePrivate
{
  Q_DECLARE_PUBLIC(CComm)

public:
  CCommPrivate();
        
  qint64 writeData(const char *data, qint64 maxSize);

  bool open(QIODevice::OpenMode mode);
  void close();
  bool flush();
  bool clear(CComm::Directions directions);

  bool comminit();

  qint64 readBufferMaxSize = 0;
  QString systemLocation;
  bool settingsRestoredOnClose = true;
  bool isBreakEnabled = false;

  bool startAsyncRead();

  bool waitForReadyRead(int msec);
  bool waitForBytesWritten(int msec);

  OVERLAPPED *waitForNotified(QDeadlineTimer deadline);

  // qint64 queuedBytesCount() const;

  bool completeAsyncCommunication(qint64 bytesTransferred);
  bool completeAsyncRead(qint64 bytesTransferred);
  bool completeAsyncWrite(qint64 bytesTransferred);

  bool startAsyncCommunication();
  bool _q_startAsyncWrite();
  void _q_notified(DWORD numberOfBytes, DWORD errorCode, OVERLAPPED *overlapped);

  qint64 queuedBytesCount(CComm::Direction direction) const;
  CComm::CommPortError getSystemError(int systemErrorCode = -1) const;

  void emitReadyRead();
  void setError(CComm::CommPortError errCode);

  QByteArray readChunkBuffer;
  QByteArray writeChunkBuffer;

  QWinOverlappedIoNotifier *notifier = nullptr;
  QTimer *startAsyncWriteTimer = nullptr;
  OVERLAPPED communicationOverlapped;
  OVERLAPPED readCompletionOverlapped;
  OVERLAPPED writeCompletionOverlapped;
  DWORD triggeredEventMask = 0;

  bool communicationStarted = false;
  bool writeStarted = false;
  bool readStarted = false;

  CComm::CommPortError errVar = CComm::NoError;

  FTTIMEOUTS currentCommTimeouts;
  FTTIMEOUTS restoredCommTimeouts;
  
  FT_HANDLE bthandle = NULL;
  FT_STATUS ftStatus; //Status defined in D2XX to indicate operation result
};

QT_END_NAMESPACE

#endif