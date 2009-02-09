/*
 * This file is part of Licq, an instant messaging client for UNIX.
 * Copyright (C) 1999-2006 Licq developers
 *
 * Licq is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Licq is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Licq; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef SIGNAL_MANAGER_H
#define SIGNAL_MANAGER_H

#include <qobject.h>

class QSocketNotifier;
class CICQDaemon;
class LicqSignal;
class ICQEvent;

//=====CSignalManager===========================================================
class CSignalManager: public QObject
{
  Q_OBJECT
public:
  CSignalManager(CICQDaemon *, int);
  ~CSignalManager();

private:
  CICQDaemon *licqDaemon;
  int m_nPipe;
  QSocketNotifier *sn;

  void ProcessSignal(LicqSignal* s);
  void ProcessEvent(ICQEvent *e);

protected slots:
  void slot_incoming();

signals:
  // Signal signals
  void signal_updatedList(unsigned long subSignal, int argument, int userId);
  void signal_updatedUser(int userId, unsigned long subSignal, int argument, unsigned long cid);
  void signal_updatedStatus(unsigned long ppid);
  void signal_logon();
  void signal_logoff();
  void signal_ui_viewevent(int userId);
  void signal_ui_message(int userId);
  void signal_protocolPlugin(unsigned long);
  void signal_eventTag(int userId, unsigned long convoId);
  void signal_socket(int userId, unsigned long convoId);
  void signal_convoJoin(int userId, unsigned long ppid, unsigned long convoId);
  void signal_convoLeave(int userId, unsigned long ppid, unsigned long convoId);
  void signal_verifyImage(unsigned long);
  void signal_newOwner(const char *, unsigned long);
  
  // Event signals
  void signal_doneOwnerFcn(ICQEvent *);
  void signal_doneUserFcn(ICQEvent *);
  void signal_searchResult(ICQEvent *);
};


#endif
