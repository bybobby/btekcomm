
#include "cwinbtek.h"
#include <sstream>
#include <string>
#include <QDebug>
#include <QTimer>
#include <private/qobject_p.h>
#include "qwinoverlappedionotifier_p.h"
#include <stdio.h>
#include <iostream>

using namespace std;

CCommPrivate::CCommPrivate()
  : readChunkBuffer(COMM_BUFFERSIZE, 0)
{
  writeBufferChunkSize = COMM_BUFFERSIZE;
  readBufferChunkSize = COMM_BUFFERSIZE;
  ::ZeroMemory(&writeCompletionOverlapped, sizeof(writeCompletionOverlapped));
  ::ZeroMemory(&readCompletionOverlapped, sizeof(readCompletionOverlapped));
  ::ZeroMemory(&communicationOverlapped, sizeof(communicationOverlapped));
}

bool CCommPrivate::open(QIODevice::OpenMode mode)
{
  DWORD numDevs;
  FT_DEVICE_LIST_INFO_NODE *devInfo;
  WCHAR Buf[64];

  ftStatus = FT_CreateDeviceInfoList(&numDevs);

  if (ftStatus == FT_OK)
    qDebug() << "Number of devices is " << numDevs;
  else
    return FALSE;

  if (numDevs > 0) {
    // allocate storage for list based on numDevs
    devInfo = (FT_DEVICE_LIST_INFO_NODE*)malloc(sizeof(FT_DEVICE_LIST_INFO_NODE)*numDevs);
    // get the device information list
    ftStatus = FT_GetDeviceInfoList(devInfo, &numDevs);
    if (ftStatus == FT_OK) {
      // for (int i = 0; i < numDevs; i++) {
      //   qDebug() << "Dev :" << i;
      //   qDebug() << "Flags=0x" << devInfo[i].Flags;
      //   qDebug() << "Type=0x" << devInfo[i].Type;
      //   qDebug() << "ID=0x" << devInfo[i].ID;
      //   qDebug() << "LocId=0x" << devInfo[i].LocId;
      //   qDebug() << "SerialNumber = " << devInfo[i].SerialNumber;
      //   qDebug() << "Description = " << devInfo[i].Description;
      //   qDebug() << "ftHandle = " << devInfo[i].ftHandle;
      // }
    }
  }
  else {
    qDebug() << "Device number is under 0 ";
    return FALSE;
  }

  DWORD desiredAccess = 0;

  if (mode & QIODevice::ReadOnly)
    desiredAccess |= GENERIC_READ;
  if (mode & QIODevice::WriteOnly)
    desiredAccess |= GENERIC_WRITE;

  ftStatus = FT_ListDevices(0, Buf, FT_LIST_BY_INDEX | FT_OPEN_BY_SERIAL_NUMBER);
  bthandle = FT_W32_CreateFile(Buf, desiredAccess, 0, nullptr,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FT_OPEN_BY_SERIAL_NUMBER, nullptr);
  
  if (bthandle == INVALID_HANDLE_VALUE)
  {
    qDebug() << "Can't open FT4232H device";
    return FALSE;
  }
  else // Port opened successfully
    qDebug() << "Successfully open FT4232H device! ";
  
  DWORD dwCount;
  ftStatus = FT_ResetDevice(bthandle); //Reset USB device
                                       //Purge USB receive buffer first by reading out all old data from FT2232H receive buffer

  BOOL w32status = FALSE;
  FTTIMEOUTS ftTS;

  ftTS.ReadIntervalTimeout = 0;
  ftTS.ReadTotalTimeoutMultiplier = 0;
  ftTS.ReadTotalTimeoutConstant = 1000;
  ftTS.WriteTotalTimeoutMultiplier = 0;
  ftTS.WriteTotalTimeoutConstant = 1000;

  // ftStatus |= FT_GetQueueStatus(ftHandle, &dwNumInputBuffer); // Get the number of bytes in the FT2232H receive buffer
  // if ((ftStatus == FT_OK) && (dwNumInputBuffer > 0))
    // ftStatus |= ft_uread(ftHandle, InputBuffer, dwNumInputBuffer, &dwNumBytesRead); //Read out the data from FT2232H receive buffer

  ftStatus |= FT_SetUSBParameters(bthandle, 1572863, 1572863); //Set USB request transfer size
  w32status |= FT_W32_SetupComm(bthandle, 1572863, 1572863); //Set USB request transfer size
  ftStatus |= FT_SetChars(bthandle, false, 0, false, 0); //Disable event and error characters
  ftStatus |= FT_SetTimeouts(bthandle, 1000, 1000); //Sets the read and write timeouts in 3 sec for the FT2232H
  w32status |= FT_W32_SetCommTimeouts(bthandle, &ftTS);
  ftStatus |= FT_SetLatencyTimer(bthandle, 1); //Set the latency timer
  ftStatus |= FT_SetBitMode(bthandle, 0x0, 0x00); //Reset controller
  ftStatus |= FT_SetBitMode(bthandle, 0x0, 0x02); //Enable MPSSE mode

  if (ftStatus != FT_OK || !w32status)
  {
    qDebug() << "fail on initialize FT4232H device !";
    return FALSE;
  }

  if (comminit())
  {
    return TRUE;
  }

  FT_W32_CloseHandle(bthandle);

  return FALSE;
}

void CCommPrivate::close()
{
  ::CancelIo(bthandle);

  delete notifier;
  notifier = nullptr;

  delete startAsyncWriteTimer;
  startAsyncWriteTimer = nullptr;

  communicationStarted = false;
  readStarted = false;
  writeStarted = false;
  writeBuffer.clear();

  FT_W32_CloseHandle(bthandle);
  
  bthandle = INVALID_HANDLE_VALUE;
}

bool CCommPrivate::comminit()
{  
  Q_Q(CComm);

  if (!FT_W32_GetCommTimeouts(bthandle, &restoredCommTimeouts)) {
    qDebug() << "GetCommTimeouts failed!! ";
    return false;
  }
  
  ::ZeroMemory(&currentCommTimeouts, sizeof(currentCommTimeouts));
  currentCommTimeouts.ReadIntervalTimeout = MAXDWORD;
  
  if (!FT_W32_SetCommTimeouts(bthandle, &currentCommTimeouts)) {
    qDebug() << "Error FT_W32_SetCommTimeouts";
    return FALSE;
  }
  
  const DWORD eventMask = EV_RXCHAR;
  
  if (!FT_W32_SetCommMask(bthandle, eventMask)) {
    return FALSE;
  }

  notifier = new QWinOverlappedIoNotifier(q);
  QObjectPrivate::connect(notifier, &QWinOverlappedIoNotifier::notified,
    this, &CCommPrivate::_q_notified);
  notifier->setHandle(bthandle);
  notifier->setEnabled(true);

  if ((eventMask & EV_RXCHAR) && !startAsyncCommunication()) {
    delete notifier;
    notifier = nullptr;
    return FALSE;
  }

  return TRUE;
}

void CCommPrivate::_q_notified(DWORD numberOfBytes, DWORD errorCode, OVERLAPPED *overlapped)
{
  // const QSerialPortErrorInfo error = getSystemError(errorCode);
  // if (errCode != QSerialPort::NoError) {    
  //   return;
  // }

  if (overlapped == &communicationOverlapped) {
    completeAsyncCommunication(numberOfBytes);
    qDebug() << "commnication";
  }
  else if (overlapped == &readCompletionOverlapped) {
    completeAsyncRead(numberOfBytes);
    qDebug() << "read completion";
  }
  else if (overlapped == &writeCompletionOverlapped) {
    completeAsyncWrite(numberOfBytes);
    qDebug() << "write completion";
  }
  else
    Q_ASSERT(!"Unknown OVERLAPPED activated");
}


bool CCommPrivate::completeAsyncCommunication(qint64 bytesTransferred)
{
  communicationStarted = false;

  if (bytesTransferred == qint64(-1))
    return false;

  return startAsyncRead();
}

bool CCommPrivate::completeAsyncRead(qint64 bytesTransferred)
{
  if (bytesTransferred == qint64(-1)) {
    readStarted = false;
    return false;
  }
  if (bytesTransferred > 0)
    buffer.append(readChunkBuffer.constData(), bytesTransferred);

  readStarted = false;

  bool result = true;
  if (bytesTransferred == COMM_BUFFERSIZE
    || queuedBytesCount(CComm::Input) > 0) {
    result = startAsyncRead();
  }
  else {
    result = startAsyncCommunication();
  }

  if (bytesTransferred > 0)
    emitReadyRead();

  return result;
}

bool CCommPrivate::completeAsyncWrite(qint64 bytesTransferred)
{
  Q_Q(CComm);

  if (writeStarted) {
    if (bytesTransferred == qint64(-1)) {
      writeChunkBuffer.clear();
      writeStarted = false;
      return false;
    }
    Q_ASSERT(bytesTransferred == writeChunkBuffer.size());
    writeChunkBuffer.clear();
    emit q->bytesWritten(bytesTransferred);
    writeStarted = false;
  }

  return _q_startAsyncWrite();
}

bool CCommPrivate::startAsyncCommunication()
{
  if (communicationStarted)
    return true;

  ::ZeroMemory(&communicationOverlapped, sizeof(communicationOverlapped));
  if ((FT_W32_WaitCommEvent(bthandle, &triggeredEventMask, &communicationOverlapped)) != FT_OK) {
    CComm::CommPortError errCode = getSystemError();
    if (errCode != CComm::NoError) {
      if (errCode == CComm::PermissionError)
        errCode = CComm::ResourceError;
        setError(errCode);
      return false;
    }
  }
  communicationStarted = true;
  return true;
}


qint64 CCommPrivate::queuedBytesCount(CComm::Direction direction) const
{
  COMSTAT comstat;
  DWORD ErrorVal = 0;

  if (FT_W32_ClearCommError(bthandle, &ErrorVal, (LPFTCOMSTAT)&comstat) == 0)
    return -1;
  return (direction == CComm::Input)
    ? comstat.cbInQue
    : ((direction == CComm::Output) ? comstat.cbOutQue : -1);
}

CComm::CommPortError CCommPrivate::getSystemError(int systemErrorCode) const
{
  if (systemErrorCode == -1)
    systemErrorCode = FT_W32_GetLastError(bthandle);

  CComm::CommPortError errCode;  

  switch (systemErrorCode) {
  case ERROR_SUCCESS:
    errCode = CComm::NoError;
    break;
  case ERROR_IO_PENDING:
    errCode = CComm::NoError;
    break;
  case ERROR_MORE_DATA:
    errCode = CComm::NoError;
    break;
  case ERROR_FILE_NOT_FOUND:
    errCode = CComm::DeviceNotFoundError;
    break;
  case ERROR_PATH_NOT_FOUND:
    errCode = CComm::DeviceNotFoundError;
    break;
  case ERROR_INVALID_NAME:
    errCode = CComm::DeviceNotFoundError;
    break;
  case ERROR_ACCESS_DENIED:
    errCode = CComm::PermissionError;
    break;
  case ERROR_INVALID_HANDLE:
    errCode = CComm::ResourceError;
    break;
  case ERROR_INVALID_PARAMETER:
    errCode = CComm::UnsupportedOperationError;
    break;
  case ERROR_BAD_COMMAND:
    errCode = CComm::ResourceError;
    break;
  case ERROR_DEVICE_REMOVED:
    errCode = CComm::ResourceError;
    break;
  case ERROR_OPERATION_ABORTED:
    errCode = CComm::ResourceError;
    break;
  case WAIT_TIMEOUT:
    errCode = CComm::TimeoutError;
    break;
  default:
    errCode = CComm::UnknownError;
    break;
  }
  
  return errCode;
}

bool CCommPrivate::startAsyncRead()
{
  if (readStarted)
    return true;

  qint64 bytesToRead = COMM_BUFFERSIZE;  
  DWORD ReadBytes = 0;

  if (readBufferMaxSize && bytesToRead > (readBufferMaxSize - buffer.size())) {
    bytesToRead = readBufferMaxSize - buffer.size();
    if (bytesToRead <= 0) {
      // Buffer is full. User must read data from the buffer
      // before we can read more from the port.
      return false;
    }
  }

  Q_ASSERT(int(bytesToRead) <= readChunkBuffer.size());

  ::ZeroMemory(&readCompletionOverlapped, sizeof(readCompletionOverlapped));
  
  if (FT_W32_ReadFile(bthandle, readChunkBuffer.data(), bytesToRead, &ReadBytes, &readCompletionOverlapped)) {
    readStarted = true;
    return true;
  }

  CComm::CommPortError errCode = getSystemError();
  if (errCode != CComm::NoError) {
    if (errCode == CComm::PermissionError)
      errCode = CComm::ResourceError;
    if (errCode != CComm::ResourceError)
      errCode = CComm::ReadError;
    setError(errCode);
    return false;
  }

  readStarted = true;
  return true;
}


bool CCommPrivate::_q_startAsyncWrite()
{
  if (writeBuffer.isEmpty() || writeStarted)
    return true;

  DWORD byteWritten = 0;
  writeChunkBuffer = writeBuffer.read();
  ::ZeroMemory(&writeCompletionOverlapped, sizeof(writeCompletionOverlapped));
  if ((FT_W32_WriteFile(bthandle, (LPVOID)writeChunkBuffer.constData(),
          writeChunkBuffer.size(), &byteWritten, &writeCompletionOverlapped)) != FT_OK) {
  
    CComm::CommPortError errCode = getSystemError();
    if (errCode != CComm::NoError) {
      if (errCode != CComm::ResourceError)
        errCode = CComm::WriteError;
      setError(errCode);
      return false;
    }
  }

  writeStarted = true;
  return true;
}

void CCommPrivate::emitReadyRead()
{
  Q_Q(CComm);

  emit q->readyRead();
}


qint64 CCommPrivate::writeData(const char *data, qint64 maxSize)
{
  Q_Q(CComm);

  writeBuffer.append(data, maxSize);

  if (!writeBuffer.isEmpty() && !writeStarted) {
    if (!startAsyncWriteTimer) {
      startAsyncWriteTimer = new QTimer(q);
      QObjectPrivate::connect(startAsyncWriteTimer, &QTimer::timeout, this, &CCommPrivate::_q_startAsyncWrite);
      startAsyncWriteTimer->setSingleShot(true);
    }
    if (!startAsyncWriteTimer->isActive())
      startAsyncWriteTimer->start();
  }
  return maxSize;
}

OVERLAPPED *CCommPrivate::waitForNotified(QDeadlineTimer deadline)
{
  OVERLAPPED *overlapped = notifier->waitForAnyNotified(deadline);
  if (!overlapped) {
    setError(getSystemError(WAIT_TIMEOUT));
    return nullptr;
  }
  return overlapped;
}

bool CCommPrivate::waitForReadyRead(int msecs)
{
  if (!writeStarted && !_q_startAsyncWrite())
    return false;

  const qint64 initialReadBufferSize = buffer.size();
  qint64 currentReadBufferSize = initialReadBufferSize;

  QDeadlineTimer deadline(msecs);

  do {
    const OVERLAPPED *overlapped = waitForNotified(deadline);
    if (!overlapped)
      return false;

    if (overlapped == &readCompletionOverlapped) {
      const qint64 readBytesForOneReadOperation = qint64(buffer.size()) - currentReadBufferSize;
      if (readBytesForOneReadOperation == COMM_BUFFERSIZE) {
        currentReadBufferSize = buffer.size();
      }
      else if (readBytesForOneReadOperation == 0) {
        if (initialReadBufferSize != currentReadBufferSize)
          return true;
      }
      else {
        return true;
      }
    }

  } while (!deadline.hasExpired());

  return false;
}

bool CCommPrivate::waitForBytesWritten(int msecs)
{
  if (writeBuffer.isEmpty() && writeChunkBuffer.isEmpty())
    return false;

  if (!writeStarted && !_q_startAsyncWrite())
    return false;

  QDeadlineTimer deadline(msecs);

  for (;;) {
    const OVERLAPPED *overlapped = waitForNotified(deadline);
    if (!overlapped)
      return false;

    if (overlapped == &writeCompletionOverlapped)
      return true;
  }

  return false;
}

bool CCommPrivate::flush()
{
  return _q_startAsyncWrite();
}

bool CCommPrivate::clear(CComm::Directions directions)
{
  DWORD flags = 0;
  if (directions & CComm::Input)
    flags |= PURGE_RXABORT | PURGE_RXCLEAR;
  if (directions & CComm::Output)
    flags |= PURGE_TXABORT | PURGE_TXCLEAR;
  if (!FT_W32_PurgeComm(bthandle, flags)) {
    setError(getSystemError());
    return false;
  }

  // We need start async read because a reading can be stalled. Since the
  // PurgeComm can abort of current reading sequence, or a port is in hardware
  // flow control mode, or a port has a limited read buffer size.
  if (directions & CComm::Input)
    startAsyncCommunication();

  return true;
}

CComm::FTHandle CComm::bthandle() const
{
  Q_D(const CComm);
  return d->bthandle;
}