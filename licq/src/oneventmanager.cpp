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

#include "oneventmanager.h"

#include <licq_constants.h>
#include <licq_icq.h>
#include <licq/contactlist/owner.h>
#include <licq/contactlist/user.h>
#include <licq/contactlist/usermanager.h>
#include <licq/inifile.h>

#include <cstdlib> // system, free
#include <ctime> // time

using namespace std;
using namespace LicqDaemon;
using Licq::gUserManager;


// Declare global OnEventManager (internal for daemon)
LicqDaemon::OnEventManager LicqDaemon::gOnEventManager;

// Declare global Licq::OnEventManager to refer to the internal OnEventManager
Licq::OnEventManager& Licq::gOnEventManager(LicqDaemon::gOnEventManager);


OnEventManager::OnEventManager()
  : myEnabled(true)
{
  // Empty
}

OnEventManager::~OnEventManager()
{
  // Empty
}

void OnEventManager::initialize()
{
  Licq::IniFile licqConf("licq.conf");
  licqConf.loadFile();

  string soundDir = SHARE_DIR;
  soundDir += "sounds/icq/";

  licqConf.setSection("onevent");
  licqConf.get("Enable", myEnabled, true);
  licqConf.get("AlwaysOnlineNotify", myAlwaysOnlineNotify, false);
  licqConf.get("Command", myCommand, "play");
  licqConf.get("Message", myParameters[OnEventMessage], soundDir + "Message.wav");
  licqConf.get("Url", myParameters[OnEventUrl], soundDir + "URL.wav");
  licqConf.get("Chat", myParameters[OnEventChat], soundDir + "Chat.wav");
  licqConf.get("File", myParameters[OnEventFile], soundDir + "File.wav");
  licqConf.get("OnlineNotify", myParameters[OnEventOnline], soundDir + "Online.wav");
  licqConf.get("SysMsg", myParameters[OnEventSysMsg], soundDir + "System.wav");
  licqConf.get("MsgSent", myParameters[OnEventMsgSent], soundDir + "Message.wav");
  licqConf.get("Sms", myParameters[OnEventSms], soundDir + "Message.wav");
}

void OnEventManager::lock()
{
  myMutex.lock();
}

void OnEventManager::unlock(bool save)
{
  if (save)
  {
    Licq::IniFile licqConf("licq.conf");
    if (!licqConf.loadFile())
      return;

    licqConf.setSection("onevent");
    licqConf.set("Enable", myEnabled);
    licqConf.set("AlwaysOnlineNotify", myAlwaysOnlineNotify);
    licqConf.set("Command", myCommand);
    licqConf.set("Message", myParameters[OnEventMessage]);
    licqConf.set("Url", myParameters[OnEventUrl]);
    licqConf.set("Chat", myParameters[OnEventChat]);
    licqConf.set("File", myParameters[OnEventFile]);
    licqConf.set("OnlineNotify", myParameters[OnEventOnline]);
    licqConf.set("SysMsg", myParameters[OnEventSysMsg]);
    licqConf.set("MsgSent", myParameters[OnEventMsgSent]);
    licqConf.set("Sms", myParameters[OnEventSms]);
    licqConf.writeFile();
  }

  myMutex.unlock();
}

bool OnEventManager::enabled() const
{
  return myEnabled;
}

void OnEventManager::setEnabled(bool enabled)
{
  myEnabled = enabled;
}

bool OnEventManager::alwaysOnlineNotify() const
{
  return myAlwaysOnlineNotify;
}

void OnEventManager::setAlwaysOnlineNotify(bool alwaysOnlineNotify)
{
  myAlwaysOnlineNotify = alwaysOnlineNotify;
}

string OnEventManager::command() const
{
  return myCommand;
}

void OnEventManager::setCommand(const string& command)
{
  myCommand = command;
}

string OnEventManager::parameter(OnEventType event) const
{
  return myParameters[event];
}

void OnEventManager::setParameter(OnEventType event, const string& parameter)
{
  myParameters[event] = parameter;
}

void OnEventManager::performOnEvent(OnEventType event, const Licq::User* user)
{
  if (!myEnabled)
    return;

  if (user != NULL)
  {
    if (user->onEventsBlocked())
      return;

    if (event == OnEventOnline)
    {
      if (!user->OnlineNotify())
        return;

      // We cannot always differentiate from users going online and users
      //   already online when we sign on. Make a guess based on online since.
      if (!myAlwaysOnlineNotify && user->OnlineSince()+60 < time(NULL))
        return;
    }

    // Get owner for user
    const Licq::Owner* owner;
    if (user->isUser())
      owner = gUserManager.FetchOwner(user->ppid(), LOCK_R);
    else
      owner = dynamic_cast<const Licq::Owner*>(user);
    if (owner == NULL)
      return;

    // Check if current status is reason to block on event action
    bool blocked = false;
    switch (owner->Status())
    {
      case ICQ_STATUS_AWAY:
        blocked = (!owner->AcceptInAway() && !user->AcceptInAway());
        break;
      case ICQ_STATUS_NA:
        blocked = (!owner->AcceptInNA() && !user->AcceptInNA());
        break;
      case ICQ_STATUS_OCCUPIED:
        blocked = (!owner->AcceptInOccupied() && !user->AcceptInOccupied());
        break;
      case ICQ_STATUS_DND:
        blocked = (!owner->AcceptInDND() && !user->AcceptInDND());
        break;
    }

    if (user->isUser())
      gUserManager.DropOwner(owner);

    if (blocked)
      return;
  }

  myMutex.lock();

  string param = myParameters[event];
  if (user != NULL)
  {
    char* newParam = user->usprintf(param.c_str(), Licq::USPRINTF_LINEISCMD);
    param = newParam;
    free(newParam);
  }
  if (!param.empty())
  {
    string fullCmd = myCommand + " " + param + " &";
    system(fullCmd.c_str());
  }

  myMutex.unlock();
}