// -*- c-basic-offset: 2 -*-
/* ----------------------------------------------------------------------------
 * Licq - A ICQ Client for Unix
 * Copyright (C) 1998 - 2009 Licq developers
 *
 * This program is licensed under the terms found in the LICENSE file.
 */

#include "config.h"

#include <licq/utility.h>

#include <cerrno>
#include <ctime>
#include <ctype.h>
#include <dirent.h>
#include <paths.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <licq/contactlist/user.h>
#include <licq/inifile.h>
#include <licq/log.h>

#include "gettext.h"
#include "support.h"

using namespace std;
using Licq::Utility;
using Licq::UtilityInternalWindow;
using Licq::UtilityManager;
using Licq::UtilityUserField;

Licq::UtilityManager Licq::gUtilityManager;


int SelectUtility(const struct dirent *d)
{
  const char* pcDot = strrchr(d->d_name, '.');
  if (pcDot == NULL) return (0);
  return (strcmp(pcDot, ".utility") == 0);
}

UtilityManager::UtilityManager()
{
  // Empty
}

UtilityManager::~UtilityManager()
{
  std::vector<Utility*>::iterator iter;
  for (iter = myUtilities.begin(); iter != myUtilities.end(); ++iter)
    delete *iter;
}

int UtilityManager::loadUtilities(const string& dir)
{
  struct dirent **namelist;

  gLog.Info(tr("%sLoading utilities.\n"), L_INITxSTR);
  int n = scandir_alpha_r(dir.c_str(), &namelist, SelectUtility);
  if (n < 0)
  {
    gLog.error("Error reading utility directory \"%s\":\n%s",
        dir.c_str(), strerror(errno));
    return (0);
  }

  Utility* p;
  for (unsigned short i = 0; i < n; i++)
  {
    string filename = dir + "/" + namelist[i]->d_name;
    free (namelist[i]);
    p = new Utility(filename);
    if (p->isFailed())
    {
      gLog.Warn(tr("%sWarning: unable to load utility \"%s\".\n"), L_WARNxSTR, namelist[i]->d_name);
      continue;
    }
    myUtilities.push_back(p);
  }
  free(namelist);

  return myUtilities.size();
}


Utility::Utility(const string& filename)
{
  // Assumes the given filename is in the form <directory>/<pluginname>.plugin
  myIsFailed = false;
  Licq::IniFile utilConf(filename);
  if (!utilConf.loadFile())
  {
    myIsFailed = true;
    return;
  }

  utilConf.setSection("utility");

  // Read in the window
  string window;
  utilConf.get("Window", window, "GUI");
  if (window == "GUI")
    myWinType = WinGui;
  else if (window ==  "TERM")
    myWinType = WinTerm;
  else if (window == "LICQ")
    myWinType = WinLicq;
  else
  {
    gLog.Warn(tr("%sWarning: Invalid entry in plugin \"%s\":\nWindow = %s\n"),
        L_WARNxSTR, filename.c_str(), window.c_str());
    myIsFailed = true;
    return;
  }

  // Read in the command
  if (!utilConf.get("Command", myCommand))
  {
    myIsFailed = true;
    return;
  }
  utilConf.get("Description", myDescription, tr("none"));

  // Parse command for %# user fields
  size_t pcField = 0;
  int nField, nCurField = 1;
  while ((pcField = myCommand.find('%', pcField)) != string::npos)
  {
    char cField = myCommand[pcField + 1];
    if (isdigit(cField))
    {
      nField = cField - '0';
      if (nField == 0 || nField > nCurField)
      {
        gLog.Warn("%sWarning: Out-of-order user field id (%d) in plugin \"%s\".\n",
            L_WARNxSTR, nField, filename.c_str());
      }
      else if (nField == nCurField)
      {
        char key[30];
        string title, defaultValue;
        sprintf(key, "User%d.Title", nField);
        utilConf.get(key, title, "User field");
        sprintf(key, "User%d.Default", nField);
        utilConf.get(key, defaultValue, "");
        myUserFields.push_back(new UtilityUserField(title, defaultValue));
        nCurField = nField + 1;
      }
    }
    pcField += 2;
  }

  size_t startPos = filename.rfind('/');
  size_t endPos = filename.rfind('.');
  if (startPos == string::npos)
    startPos = 0;
  else
    ++startPos;
  if (endPos == string::npos)
    myName = filename.substr(startPos);
  else
    myName = filename.substr(startPos, endPos - startPos);
}


Utility::~Utility()
{
  std::vector<UtilityUserField *>::iterator iter;
  for (iter = myUserFields.begin(); iter != myUserFields.end(); ++iter)
    delete *iter;
}

bool Utility::setFields(const UserId& userId)
{
  Licq::UserReadGuard u(userId);
  if (!u.isLocked())
    return false;
  myFullCommand = u->usprintf(myCommand, Licq::User::usprintf_quoteall, false, false);
  vector<UtilityUserField *>::iterator iter;
  for (iter = myUserFields.begin(); iter != myUserFields.end(); ++iter)
    (*iter)->setFields(*u);
  return true;
}

void Utility::setUserFields(const vector<string>& userFields)
{
  if (static_cast<int>(userFields.size()) != numUserFields())
  {
    gLog.Warn("%sInternal error: Utility::setUserFields(): incorrect number of data fields (%d/%d).\n",
        L_WARNxSTR, int(userFields.size()), numUserFields());
    return;
  }
  // Do a quick check to see if there are any users fields at all
  if (numUserFields() == 0)
    return;

  size_t pcField;
  while ((pcField = myFullCommand.find('%')) != string::npos)
  {
    char c = myFullCommand[pcField+1];
    if (isdigit(c))
    {
      // We know that any user field numbers are valid from the constructor
      myFullCommand.replace(pcField, 2,  userFields[c - '1']);
    }
    else
    {
      // Anything non-digit at this point we just ignore
      myFullCommand.erase(pcField, 2);
    }
  }
}


UtilityUserField::UtilityUserField(const string& title, const string& defaultValue)
  : myTitle(title),
    myDefaultValue(defaultValue)
{
  // Empty
}

UtilityUserField::~UtilityUserField()
{
  // Empty
}

bool UtilityUserField::setFields(const User* u)
{
  myFullDefault = u->usprintf(myDefaultValue, Licq::User::usprintf_quoteall, false, false);
  return true;
}


UtilityInternalWindow::UtilityInternalWindow()
{
  fStdOut = fStdErr = NULL;
  pid = -1;
}

UtilityInternalWindow::~UtilityInternalWindow()
{
  if (Running()) PClose();
}

bool UtilityInternalWindow::POpen(const string& command)
{
  int pdes_out[2], pdes_err[2];

  if (pipe(pdes_out) < 0) return false;
  if (pipe(pdes_err) < 0) return false;

  switch (pid = fork())
  {
    case -1:                        /* Error. */
    {
      close(pdes_out[0]);
      close(pdes_out[1]);
      close(pdes_err[0]);
      close(pdes_err[1]);
      return false;
      /* NOTREACHED */
    }
    case 0:                         /* Child. */
    {
      if (pdes_out[1] != STDOUT_FILENO)
      {
        dup2(pdes_out[1], STDOUT_FILENO);
        close(pdes_out[1]);
      }
      close(pdes_out[0]);
      if (pdes_err[1] != STDERR_FILENO)
      {
        dup2(pdes_err[1], STDERR_FILENO);
        close(pdes_err[1]);
      }
      close(pdes_err[0]);
      execl(_PATH_BSHELL, "sh", "-c", command.c_str(), NULL);
      _exit(127);
      /* NOTREACHED */
    }
  }

  /* Parent; assume fdopen can't fail. */
  fStdOut = fdopen(pdes_out[0], "r");
  close(pdes_out[1]);
  fStdErr = fdopen(pdes_err[0], "r");
  close(pdes_err[1]);

  // Set both streams to line buffered
  setvbuf(fStdOut, (char*)NULL, _IOLBF, 0);
  setvbuf(fStdErr, (char*)NULL, _IOLBF, 0);

  return true;
}


int UtilityInternalWindow::PClose()
{
   int r, pstat;
   struct timeval tv = { 0, 200000 };

   // Close the file descriptors
   fclose(fStdOut);
   fclose(fStdErr);
   fStdOut = fStdErr = NULL;

   // See if the child is still there
   r = waitpid(pid, &pstat, WNOHANG);
   // Return if child has exited or there was an error
   if (r == pid || r == -1) goto pclose_leave;

   // Give the process another .2 seconds to die
   select(0, NULL, NULL, NULL, &tv);

   // Still there?
   r = waitpid(pid, &pstat, WNOHANG);
   if (r == pid || r == -1) goto pclose_leave;

   // Try and kill the process
   if (kill(pid, SIGTERM) == -1) return -1;

   // Give it 1 more second to die
   tv.tv_sec = 1;
   tv.tv_usec = 0;
   select(0, NULL, NULL, NULL, &tv);

   // See if the child is still there
   r = waitpid(pid, &pstat, WNOHANG);
   if (r == pid || r == -1) goto pclose_leave;

   // Kill the bastard
   kill(pid, SIGKILL);
   // Now he will die for sure
   r = waitpid(pid, &pstat, 0);

pclose_leave:

   if (r == -1 || !WIFEXITED(pstat))
     return -1;
   return WEXITSTATUS(pstat);

}
