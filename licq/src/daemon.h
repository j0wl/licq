/*
 * This file is part of Licq, an instant messaging client for UNIX.
 * Copyright (C) 2010 Licq developers
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

#ifndef LICQDAEMON_DAEMON_H
#define LICQDAEMON_DAEMON_H

#include <licq/daemon.h>

#include <licq/thread/mutex.h>

class CLicq;

namespace LicqDaemon
{

class Daemon : public Licq::Daemon
{
public:
  Daemon();
  ~Daemon();

  /**
   * Initialize the daemon
   */
  void initialize(CLicq* licq);

  /**
   * Get next available id to use for an event
   * TODO: Move to ProtocolManager when no longer used directy by ICQ code
   */
  unsigned long getNextEventId();

  /**
   * Only called by Shutdown_tep
   */
  void shutdownPlugins();

  // From Licq::Daemon
  pthread_t* Shutdown();
  const char* Version() const;
  Licq::LogService& getLogService();

private:
  unsigned long myNextEventId;
  Licq::Mutex myNextEventIdMutex;

  pthread_t thread_shutdown;

  CLicq* licq;
};

extern Daemon gDaemon;

} // namespace LicqDaemon

#endif