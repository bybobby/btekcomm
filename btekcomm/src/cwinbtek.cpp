

#include "cwinbtek.h"
#include <QDebug>
#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

CComm::CComm(QObject *parent)
  : QIODevice(*new CCommPrivate, parent)
  , d_dummy(0)
{
}

CComm::~CComm()
{
  if (isOpen())
    close();
}


bool CComm::open(OpenMode mode)
{
  Q_D(CComm);

  if (isOpen()) {
    d->setError(CComm::OpenError);
    return false;
  }

  // Define while not supported modes.
  static const OpenMode unsupportedModes = Append | Truncate | Text | Unbuffered;
  if ((mode & unsupportedModes) || mode == NotOpen) {
    d->setError(CComm::UnsupportedOperationError);
    return false;
  }

  clearError();
  if (!d->open(mode))
    return false;

  QIODevice::open(mode);
  return true;
}

void CComm::close()
{
  Q_D(CComm);

  if (!isOpen()) {
    d->setError(CComm::NotOpenError);
    return;
  }

  d->close();
  QIODevice::close();
}

void CComm::clearError()
{
  Q_D(CComm);
  d->setError(CComm::NoError);
}


void CCommPrivate::setError(CComm::CommPortError errCode)
{
  Q_Q(CComm);

  errVar = errCode;
  emit q->errorOccurred(errVar);
  emit q->error(errVar);
}

void CComm::setReadBufferSize(qint64 size)
{
  Q_D(CComm);
  d->readBufferMaxSize = size;
  if (isReadable())
    d->startAsyncRead();
}

bool CComm::waitForReadyRead(int msecs)
{
  Q_D(CComm);
  return d->waitForReadyRead(msecs);
}

qint64 CComm::readData(char *data, qint64 maxSize)
{
  Q_UNUSED(data);
  Q_UNUSED(maxSize);

  // In any case we need to start the notifications if they were
  // disabled by the read handler. If enabled, next call does nothing.
  d_func()->startAsyncRead();

  // return 0 indicating there may be more data in the future
  return qint64(0);
}

/*!
    \reimp
*/
qint64 CComm::readLineData(char *data, qint64 maxSize)
{
  return QIODevice::readLineData(data, maxSize);
}

/*!
    \reimp
*/
qint64 CComm::writeData(const char *data, qint64 maxSize)
{
  Q_D(CComm);
  return d->writeData(data, maxSize);
}

bool CComm::waitForBytesWritten(int msecs)
{
  Q_D(CComm);
  return d->waitForBytesWritten(msecs);
}

qint64 CComm::bytesAvailable() const
{
  return QIODevice::bytesAvailable();
}

qint64 CComm::bytesToWrite() const
{
  qint64 pendingBytes = QIODevice::bytesToWrite();
#if defined(Q_OS_WIN32)
  pendingBytes += d_func()->writeChunkBuffer.size();
#endif
  return pendingBytes;
}

bool CComm::flush()
{
  Q_D(CComm);

  if (!isOpen()) {
    d->setError(CComm::NotOpenError);
    qWarning("%s: device not open", Q_FUNC_INFO);
    return false;
  }

  return d->flush();
}

/*!
    Discards all characters from the output or input buffer, depending on
    given directions \a directions. This includes clearing the internal class buffers and
    the UART (driver) buffers. Also terminate pending read or write operations.
    If successful, returns \c true; otherwise returns \c false.

    \note The serial port has to be open before trying to clear any buffered
    data; otherwise returns \c false and sets the NotOpenError error code.
*/
bool CComm::clear(Directions directions)
{
  Q_D(CComm);

  if (!isOpen()) {
    d->setError(CComm::NotOpenError);
    qWarning("%s: device not open", Q_FUNC_INFO);
    return false;
  }

  if (directions & Input)
    d->buffer.clear();
  if (directions & Output)
    d->writeBuffer.clear();
  return d->clear(directions);
}

bool CComm::isSequential() const
{
  return true;
}

bool CComm::atEnd() const
{
  return QIODevice::atEnd();
}

bool CComm::canReadLine() const
{
  return QIODevice::canReadLine();
}

CComm::CommPortError CComm::error() const
{
  Q_D(const CComm);
  return d->errVar;
}

// #include "moc_cwinbtek.cpp"

QT_END_NAMESPACE
