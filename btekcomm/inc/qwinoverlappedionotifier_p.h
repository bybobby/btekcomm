/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWINOVERLAPPEDIONOTIFIER_P_H
#define QWINOVERLAPPEDIONOTIFIER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>
#include <qobject.h>
#include <qdeadlinetimer.h>
#include <qqueue.h>
#include <private/qobject_p.h>
#include <private/qiodevice_p.h>
#include <QtCore\qt_windows.h>

typedef struct _OVERLAPPED OVERLAPPED;

struct IOResult
{
  IOResult(DWORD n = 0, DWORD e = 0, OVERLAPPED* p = nullptr)
    : numberOfBytes(n), errorCode(e), overlapped(p)
  {}

  DWORD numberOfBytes = 0;
  DWORD errorCode = 0;
  OVERLAPPED* overlapped = nullptr;
};


QT_BEGIN_NAMESPACE

class QWinOverlappedIoNotifierPrivate;

class QWinOverlappedIoNotifier : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(QWinOverlappedIoNotifier)
    Q_DECLARE_PRIVATE(QWinOverlappedIoNotifier)
    Q_PRIVATE_SLOT(d_func(), void _q_notified())
    friend class QWinIoCompletionPort;
public:
    QWinOverlappedIoNotifier(QObject *parent = 0);
    ~QWinOverlappedIoNotifier();

    void setHandle(Qt::HANDLE h);
    Qt::HANDLE handle() const;

    void setEnabled(bool enabled);
    OVERLAPPED *waitForAnyNotified(QDeadlineTimer deadline);
    bool waitForNotified(QDeadlineTimer deadline, OVERLAPPED *overlapped);

Q_SIGNALS:
    void notified(quint32 numberOfBytes, quint32 errorCode, OVERLAPPED *overlapped);
#if !defined(Q_QDOC)
    void _q_notify();
#endif
};


class QWinIoCompletionPort;

class QWinOverlappedIoNotifierPrivate : public QObjectPrivate
{
  Q_DECLARE_PUBLIC(QWinOverlappedIoNotifier)
public:
  OVERLAPPED* waitForAnyNotified(QDeadlineTimer deadline);
  void notify(DWORD numberOfBytes, DWORD errorCode, OVERLAPPED* overlapped);
  void _q_notified();
  OVERLAPPED* dispatchNextIoResult();

  static QWinIoCompletionPort* iocp;
  static HANDLE iocpInstanceLock;
  static unsigned int iocpInstanceRefCount;
  HANDLE hHandle = INVALID_HANDLE_VALUE;
  HANDLE hSemaphore = nullptr;
  HANDLE hResultsMutex = nullptr;
  QAtomicInt waiting;
  QQueue<IOResult> results;
};

QT_END_NAMESPACE

#endif // QWINOVERLAPPEDIONOTIFIER_P_H
