/*
 * This file is part of Licq, an instant messaging client for UNIX.
 * Copyright (C) 2010 Erik Johansson <erijo@licq.org>
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

#ifndef LICQDAEMON_PLUGINEVENTHANDLER_H
#define LICQDAEMON_PLUGINEVENTHANDLER_H

#include "generalplugin.h"
#include "licq_events.h"
#include "protocolplugin.h"

#include <boost/noncopyable.hpp>

namespace LicqDaemon
{

class PluginEventHandler : private boost::noncopyable
{
public:
  PluginEventHandler(GeneralPluginsList& generalPlugins,
                     Licq::Mutex& generalPluginsMutex,
                     ProtocolPluginsList& protocolPlugins,
                     Licq::Mutex& protocolPluginsMutex);
  ~PluginEventHandler();

  void pushGeneralEvent(LicqEvent* event);
  LicqEvent* popGeneralEvent();

  void pushGeneralSignal(LicqSignal* signal);
  LicqSignal* popGeneralSignal();

  void pushProtocolSignal(LicqProtoSignal* signal, unsigned long ppid);
  LicqProtoSignal* popProtocolSignal();

private:
  GeneralPluginsList& myGeneralPlugins;
  Licq::Mutex& myGeneralPluginsMutex;

  ProtocolPluginsList& myProtocolPlugins;
  Licq::Mutex& myProtocolPluginsMutex;
};

} // namespace LicqDaemon

#endif