#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <signal.h>
#include <stdlib.h>

#include "licq.h"
#include "licq_sighandler.h"

#ifdef USE_SOCKS5
#define SOCKS
extern "C" {
#include <socks.h>
}
#endif


// define a global variable for the base directory containing the data and config subdirectories
char BASE_DIR[MAX_FILENAME_LEN];
char SHARE_DIR[MAX_FILENAME_LEN];
char LIB_DIR[MAX_FILENAME_LEN];
unsigned short DEBUG_LEVEL;


int main(int argc, char **argv)
{

#ifdef USE_SOCKS5
   SOCKSinit(argv[0]);
#endif

  licq_segv_handler();

  CLicq licq;
  if (!licq.Init(argc, argv)) return 1;
  return licq.Main();
}
