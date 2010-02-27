#include <stdio.h>
#include <locale.h>
#include <unistd.h>

#include "console.h"
#include "pluginversion.h"

#include <licq_plugin.h>

CLicqConsole *licqConsole;

const char *LP_Usage()
{
  static const char usage[] =
      "Usage:  Licq [ options ] -p console\n";
  return usage;
}

const char *LP_Name()
{
  static const char name[] = "Console";
  return name;
}


const char *LP_Version()
{
  static const char version[] = PLUGIN_VERSION_STRING;
  return version;
}

const char *LP_Status()
{
  static const char status[] = "running";
  return status;
}

const char *LP_Description()
{
  static const char desc[] = "Console plugin based on ncurses";
  return desc;
}

const char *LP_ConfigFile()
{
  return "licq_console.conf";
}

bool LP_Init(int argc, char **argv)
{
  //char *LocaleVal = new char;
  //LocaleVal = setlocale (LC_ALL, "");
  //bindtextdomain (PACKAGE, LOCALEDIR);
  //textdomain (PACKAGE);
  setlocale(LC_ALL, "");

  // parse command line for arguments
  int i = 0;
  while( (i = getopt(argc, argv, "h")) > 0)
  {
    switch (i)
    {
    case 'h':  // help
      puts(LP_Usage());
      return false;
    }
  }
  licqConsole = new CLicqConsole(argc, argv);
  return (licqConsole != NULL);
}


int LP_Main()
{
  int nResult = licqConsole->Run();
  licqConsole->Shutdown();
  delete licqConsole;
  return nResult;
}


