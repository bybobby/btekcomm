
#ifndef __CWINBRIDGETEK_P_H__

#define __CWINBRIDGETEK_P_H__

#pragma once

// #include "btekdll_global.h"
# define BTEKDLL_EXPORT

#include <QObject>
#include <QTGlobal>
#include <QIODevice>
#include <QDeadlineTimer>

QT_BEGIN_NAMESPACE

class CCommPrivate;

class BTEKDLL_EXPORT CComm : public QIODevice
{
  Q_OBJECT
  Q_DECLARE_PRIVATE(CComm)

public:

  enum Direction {
    Input = 1,
    Output = 2,
    AllDirections = Input | Output
  };
  Q_FLAG(Direction)
  Q_DECLARE_FLAGS(Directions, Direction)

  enum CommPortError {
    NoError,
    DeviceNotFoundError,
    PermissionError,
    OpenError,
    ParityError,
    FramingError,
    BreakConditionError,
    WriteError,
    ReadError,
    ResourceError,
    UnsupportedOperationError,
    UnknownError,
    TimeoutError,
    NotOpenError
  };
  Q_ENUM(CommPortError)
  
  typedef void* FTHandle;
  
  explicit CComm(QObject *parent = nullptr);
  virtual ~CComm();

  CommPortError error() const;

  bool open(OpenMode mode) override;
  void close() override;
  void clearError();

  bool setDataTerminalReady(bool set);
  bool isDataTerminalReady();

  bool setRequestToSend(bool set);
  bool isRequestToSend();

  bool flush();
  bool clear(Directions directions = AllDirections);
  bool atEnd() const override; // ### Qt6: remove me'

  qint64 readBufferSize() const;
  void setReadBufferSize(qint64 size);

  bool isSequential() const override;

  FTHandle bthandle() const;
  
  qint64 bytesAvailable() const override;
  qint64 bytesToWrite() const override;
  bool canReadLine() const override;

  bool waitForReadyRead(int msecs = 30000) override;
  bool waitForBytesWritten(int msecs = 30000) override;

Q_SIGNALS:
  // void dataTerminalReadyChanged(bool set);
  // void requestToSendChanged(bool set);
  // void breakEnabledChanged(bool set);
#if QT_DEPRECATED_SINCE(5, 8)
  void error(CComm::CommPortError serialPortError);
#endif
  void errorOccurred(CComm::CommPortError error);
  
protected:
  qint64 readData(char *data, qint64 maxSize) override;
  qint64 readLineData(char *data, qint64 maxSize) override;
  qint64 writeData(const char *data, qint64 maxSize) override;

private:
  // ### Qt6: remove me.
  CCommPrivate * const d_dummy;

  Q_DISABLE_COPY(CComm)

#if defined(Q_OS_WIN32)
  Q_PRIVATE_SLOT(d_func(), bool _q_startAsyncWrite())
  Q_PRIVATE_SLOT(d_func(), void _q_notified(quint32, quint32, OVERLAPPED*))
#endif
};

QT_END_NAMESPACE

#include "cwinbtek_p.h"

#endif