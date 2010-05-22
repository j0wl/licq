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

#include "plugineventhandler.h"

#include "gettext.h"
#include "licq_log.h"
#include "licq/thread/mutexlocker.h"

#include <boost/foreach.hpp>

using Licq::MutexLocker;
using namespace LicqDaemon;

PluginEventHandler::PluginEventHandler(GeneralPluginsList& generalPlugins,
                                       Licq::Mutex& generalPluginsMutex,
                                       ProtocolPluginsList& protocolPlugins,
                                       Licq::Mutex& protocolPluginsMutex) :
  myGeneralPlugins(generalPlugins),
  myGeneralPluginsMutex(generalPluginsMutex),
  myProtocolPlugins(protocolPlugins),
  myProtocolPluginsMutex(protocolPluginsMutex)
{
  // Empty
}

PluginEventHandler::~PluginEventHandler()
{
  // Empty
}

void PluginEventHandler::pushGeneralEvent(LicqEvent* event)
{
  MutexLocker locker(myGeneralPluginsMutex);
  BOOST_FOREACH(GeneralPlugin::Ptr plugin, myGeneralPlugins)
  {
    if (plugin->isThread(event->thread_plugin))
    {
      plugin->pushEvent(event);
      return;
    }
  }

  // If no plugin got the event, then just delete it
  delete event;
}

LicqEvent* PluginEventHandler::popGeneralEvent()
{
  MutexLocker locker(myGeneralPluginsMutex);
  BOOST_FOREACH(GeneralPlugin::Ptr plugin, myGeneralPlugins)
  {
    if (plugin->isThisThread())
      return plugin->popEvent();
  }
  return NULL;
}

void PluginEventHandler::pushGeneralSignal(LicqSignal* signal)
{
  MutexLocker locker(myGeneralPluginsMutex);
  BOOST_FOREACH(GeneralPlugin::Ptr plugin, myGeneralPlugins)
  {
    if (plugin->wantSignal(signal->Signal()))
      plugin->pushSignal(new LicqSignal(signal));
  }
  delete signal;
}

LicqSignal* PluginEventHandler::popGeneralSignal()
{
  MutexLocker locker(myGeneralPluginsMutex);
  BOOST_FOREACH(GeneralPlugin::Ptr plugin, myGeneralPlugins)
  {
    if (plugin->isThisThread())
      return plugin->popSignal();
  }
  return NULL;
}

void PluginEventHandler::pushProtocolSignal(LicqProtoSignal* signal,
                                            unsigned long ppid)
{
  MutexLocker locker(myProtocolPluginsMutex);
  BOOST_FOREACH(ProtocolPlugin::Ptr plugin, myProtocolPlugins)
  {
    if (plugin->getProtocolId() == ppid)
    {
      if (plugin->wantSignal(SIGNAL_ALL))
        plugin->pushSignal(signal);
      else
        delete signal;
      return;
    }
  }

  gLog.Info(tr("%sInvalid protocol plugin requested (%ld).\n"),
            L_ERRORxSTR, ppid);
  delete signal;
}

LicqProtoSignal* PluginEventHandler::popProtocolSignal()
{
  MutexLocker locker(myProtocolPluginsMutex);
  BOOST_FOREACH(ProtocolPlugin::Ptr plugin, myProtocolPlugins)
  {
    if (plugin->isThisThread())
      return plugin->popSignal();
  }
  return NULL;
}