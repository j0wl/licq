#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>

#include "licq_filetransfer.h"
#include "licq_log.h"
#include "licq_constants.h"
#include "licq_icqd.h"
#include "licq_translate.h"
#include "licq_sighandler.h"
#include "support.h"

#define DEBUG_THREADS(x)

const unsigned short FT_STATE_DISCONNECTED = 0;
const unsigned short FT_STATE_HANDSHAKE = 1;
const unsigned short FT_STATE_WAITxFORxCLIENTxINIT = 2;
const unsigned short FT_STATE_WAITxFORxSERVERxINIT = 3;
const unsigned short FT_STATE_WAITxFORxSTART = 4;
const unsigned short FT_STATE_WAITxFORxFILExINFO = 5;
const unsigned short FT_STATE_RECEIVINGxFILE = 6;
const unsigned short FT_STATE_SENDINGxFILE = 7;



//=====FILE==================================================================


//-----FileInitClient-----------------------------------------------------------
CPFile_InitClient::CPFile_InitClient(char *_szLocalName,
                                    unsigned long _nNumFiles,
                                    unsigned long _nTotalSize)
{
  m_nSize = 20 + strlen(_szLocalName);
  InitBuffer();

  buffer->PackChar(0x00);
  buffer->PackUnsignedLong(0);
  buffer->PackUnsignedLong(_nNumFiles);
  buffer->PackUnsignedLong(_nTotalSize);
  buffer->PackUnsignedLong(0x64);
  buffer->PackString(_szLocalName);
}


//-----FileInitServer-----------------------------------------------------------
CPFile_InitServer::CPFile_InitServer(char *_szLocalName)
{
  m_nSize = 8 + strlen(_szLocalName);
  InitBuffer();

  buffer->PackChar(0x01);
  buffer->PackUnsignedLong(0x64);
  buffer->PackString(_szLocalName);
}


//-----FileBatch----------------------------------------------------------------
CPFile_Info::CPFile_Info(const char *_szFileName)
{
  m_bValid = true;
  m_nError = 0;

  char *pcNoPath = NULL;
  struct stat buf;

  // Remove any path from the filename
  if ( (pcNoPath = strrchr(_szFileName, '/')) != NULL)
    m_szFileName = strdup(pcNoPath + 1);
  else
    m_szFileName = strdup(_szFileName);

  if (stat(_szFileName, &buf) < 0)
  {
    m_bValid = false;
    m_nError = errno;
    return;
  }
  m_nFileSize = buf.st_size;

  m_nSize = strlen(m_szFileName) + 21;
  InitBuffer();

  buffer->PackUnsignedShort(0x02);

  // Add all the file names
  buffer->PackString(m_szFileName);
  // Add the empty file name
  buffer->PackString("");
  //Add the file length
  buffer->PackUnsignedLong(m_nFileSize);
  buffer->PackUnsignedLong(0x00);
  buffer->PackUnsignedLong(0x64);
}


CPFile_Info::~CPFile_Info()
{
  free (m_szFileName);
}


//-----FileStart----------------------------------------------------------------
CPFile_Start::CPFile_Start(unsigned long _nFilePos)
{
  m_nSize = 13;
  InitBuffer();

  buffer->PackChar(0x03);
  buffer->PackUnsignedLong(_nFilePos);
  buffer->PackUnsignedLong(0x00);
  buffer->PackUnsignedLong(0x64);
}


//=====FileTransferManager===========================================================
CFileTransferManager::CFileTransferManager(CICQDaemon *d, unsigned long nUin)
{
  // Create the plugin notification pipe
  pipe(pipe_thread);
  pipe(pipe_events);

  m_nUin = nUin;
  m_nSession = rand();
  licqDaemon = d;

  ICQOwner *o = gUserManager.FetchOwner(LOCK_R);
  strcpy(m_szLocalName, o->GetAlias());
  gUserManager.DropOwner();

  m_nCurrentFile = m_nTotalFiles = 0;
  m_nFileSize = m_nBatchSize = m_nFilePos = m_nBatchPos = 0;
  m_nBytesTransfered = m_nBatchBytesTransfered = 0;
  m_nStartTime = m_nBatchStartTime = 0;
  m_nFileDesc = -1;

  m_szPathName[0] = '\0';
  m_szFileName[0] = '\0';
  m_szLocalName[0] = '\0';
  m_szRemoteName[0] = '\0';
}


//-----CFileTransferManager::StartFileTransferServer-----------------------------------------
bool CFileTransferManager::StartFileTransferServer()
{
  if (licqDaemon->StartTCPServer(&ftServer) == -1)
  {
    gLog.Warn("%sNo more ports available, add more or close open chat/file sessions.\n", L_WARNxSTR);
    return false;
  }

  // Add the server to the sock manager
  sockman.AddSocket(&ftServer);
  sockman.DropSocket(&ftServer);

  return true;
}



bool CFileTransferManager::StartAsServer()
{
  if (!StartFileTransferServer()) return false;

  // Create the socket manager thread
  if (pthread_create(&thread_ft, NULL, &FileTransferManager_tep, this) == -1)
    return false;

  return true;
}


//-----CFileTransferManager::StartAsClient-------------------------------------------
bool CFileTransferManager::StartAsClient(unsigned short nPort)
{
  //if (!StartFileTransferServer()) return false;

  if (!ConnectToFileServer(nPort)) return false;

  // Create the socket manager thread
  if (pthread_create(&thread_ft, NULL, &FileTransferManager_tep, this) == -1)
    return false;

  return true;
}


//-----CFileTransferManager::ConnectToFileServer-----------------------------
bool CFileTransferManager::ConnectToFileServer(unsigned short nPort)
{
  gLog.Info("%sFile Transfer: Connecting to server.\n", L_TCPxSTR);
  if (!licqDaemon->OpenConnectionToUser(m_nUin, &ftSock, nPort))
  {
    return false;
  }

  gLog.Info("%sFile Transfer: Shaking hands.\n", L_TCPxSTR);

  // Send handshake packet:
  CPacketTcp_Handshake p_handshake(ftSock.LocalPort());
  if (!SendPacket(&p_handshake)) return false;

  // Send init packet:
  CPFile_InitClient p(m_szLocalName, 1, m_nFileSize);
  if (!SendPacket(&p)) return false;

  gLog.Info("%sFile Transfer: Waiting for server to respond.\n", L_TCPxSTR);

  m_nState = FT_STATE_WAITxFORxSERVERxINIT;

  sockman.AddSocket(&ftSock);
  sockman.DropSocket(&ftSock);

  return true;
}


//-----CFileTransferManager::ProcessPacket-------------------------------------------
bool CFileTransferManager::ProcessPacket()
{
  if (!ftSock.RecvPacket())
  {
    char buf[128];
    if (ftSock.Error() == 0)
      gLog.Info("%sFile Transfer: Remote end disconnected.\n", L_TCPxSTR);
    else
      gLog.Warn("%sFile Transfer: Lost remote end:\n%s%s\n", L_WARNxSTR,
                L_BLANKxSTR, ftSock.ErrorStr(buf, 128));
    return false;
  }

  if (!ftSock.RecvBufferFull()) return true;
  CBuffer &b = ftSock.RecvBuffer();

  switch(m_nState)
  {
    // Server States

    case FT_STATE_HANDSHAKE:
    {
      unsigned char cHandshake = b.UnpackChar();
      if (cHandshake != ICQ_CMDxTCP_HANDSHAKE)
      {
        gLog.Warn("%sFile Transfer: Bad handshake (%04X).\n", L_WARNxSTR, cHandshake);
        break;
      }

      gLog.Info("%sFile Transfer: Received handshake.\n", L_TCPxSTR);
      m_nState = FT_STATE_WAITxFORxCLIENTxINIT;
      break;
    }

    case FT_STATE_WAITxFORxCLIENTxINIT:
    {
      unsigned char nCmd = b.UnpackChar();
      if (nCmd != 0x00)
      {
        char *pbuf;
        gLog.Error("%sFile Transfer: Invalid client init packet:\n%s%s\n",
                   L_ERRORxSTR, L_BLANKxSTR, b.print(pbuf));
        delete [] pbuf;
        return false;
      }
      b.UnpackUnsignedLong();
      m_nTotalFiles = b.UnpackUnsignedLong();
      m_nBatchSize = b.UnpackUnsignedLong();
      b.UnpackUnsignedLong();
      b.UnpackString(m_szRemoteName);

      m_nBatchStartTime = time(TIME_NOW);
      m_nBatchBytesTransfered = m_nBatchPos = 0;

      // Send response
      CPFile_InitServer p(m_szLocalName);
      if (!SendPacket(&p)) return false;

      gLog.Info("%sFile Transfer: Waiting for file info.\n", L_TCPxSTR);
      m_nState = FT_STATE_WAITxFORxFILExINFO;
      break;
    }

    case FT_STATE_WAITxFORxFILExINFO:
    {
      unsigned char nCmd = b.UnpackChar();
      if (nCmd == 0x05)
      {
        // set our speed, for now fuckem and go as fast as possible
        break;
      }
      if (nCmd != 0x02)
      {
        char *pbuf;
        gLog.Error("%sFile Transfer: Invalid file info packet:\n%s%s\n",
                   L_ERRORxSTR, L_BLANKxSTR, b.print(pbuf));
        delete [] pbuf;
        return false;
      }
      b.UnpackChar();
      b.UnpackString(m_szFileName);
      b.UnpackUnsignedShort(); // 0 length string...?
      b.UnpackChar();
      b.UnpackUnsignedLong();
      m_nFileSize = b.UnpackUnsignedLong();

      m_nBytesTransfered = 0;
      m_nCurrentFile++;

      // FIXME send plugin an event???

      // Get the local filename and set the file offset (for resume)
      // set m_szPathName, m_nFilePos, m_nFileDesc given m_szFileName
      /*if (!GetLocalFileName())
      {
        fileCancel();
        return;
      }*/
      m_szPathName[0] = '\0';
      m_nFilePos = 0;
      m_nFileDesc = -1;//FIXME!!!!!!!!!

      // Send response
      CPFile_Start p(m_nFilePos);
      if (!SendPacket(&p)) return false;

      m_nState = FT_STATE_RECEIVINGxFILE;
      break;
    }

    case FT_STATE_RECEIVINGxFILE:
    {
      // if this is the first call to this function...
      if (m_nBytesTransfered == 0)
      {
        m_nStartTime = time(TIME_NOW);
        m_nBatchPos += m_nFilePos;
        gLog.Info("%sFile Transfer: Receiving %s (%ld bytes).\n", L_TCPxSTR,
           m_szFileName, m_nFileSize);
      }

      // Write the new data to the file and empty the buffer
      CBuffer &b = ftSock.RecvBuffer();
      char nCmd = b.UnpackChar();
      if (nCmd == 0x05)
      {
        // set speed...
        break;
      }

      if (nCmd != 0x06)
      {
        gLog.Unknown("%sFile Transfer: Invalid data (%c) ignoring packet.\n",
           L_UNKNOWNxSTR, nCmd);
        break;
      }

      errno = 0;
      size_t nBytesWritten = write(m_nFileDesc, b.getDataPosRead(), b.getDataSize() - 1);
      if (nBytesWritten != b.getDataSize() - 1)
      {
        gLog.Error("%sFile Transfer: Write error:\n%s%s.\n", L_ERRORxSTR, L_BLANKxSTR,
           errno == 0 ? "Disk full (?)" : strerror(errno));
        return false;
      }

      m_nFilePos += nBytesWritten;
      m_nBytesTransfered += nBytesWritten;
      m_nBatchPos += nBytesWritten;
      m_nBatchBytesTransfered += nBytesWritten;

      int nBytesLeft = m_nFileSize - m_nFilePos;
      if (nBytesLeft > 0)
        break;

      close(m_nFileDesc);
      m_nFileDesc = -1;
      if (nBytesLeft == 0) // File transfer done perfectly
      {
        gLog.Info("%sFile Transfer: %s received.\n", L_TCPxSTR, m_szFileName);
      }
      else // nBytesLeft < 0
      {
        // Received too many bytes for the given size of the current file
        gLog.Warn("%sFile Transfer: %s received %d too many bytes.\n", L_WARNxSTR,
           m_szFileName, -nBytesLeft);
      }
      // Notify Plugin
      //...FIXME

      // Now wait for a disconnect or another file
      m_nState = FT_STATE_WAITxFORxFILExINFO;

      break;
    }


    // Client States

    case FT_STATE_WAITxFORxSERVERxINIT:
    {
      char nCmd = b.UnpackChar();
      if (nCmd == 0x05)
      {
        // set our speed, for now fuckem and go as fast as possible
        break;
      }
      if (nCmd != 0x01)
      {
        char *pbuf;
        gLog.Error("%sFile Transfer: Invalid server init packet:\n%s%s\n",
                   L_ERRORxSTR, L_BLANKxSTR, b.print(pbuf));
        delete [] pbuf;
        return false;
      }
      b.UnpackUnsignedLong();
      b.UnpackString(m_szRemoteName);

      // Send file info packet
      CPFile_Info p(m_szPathName);
      if (!p.IsValid())
      {
        gLog.Warn("%sFile Transfer: Read error for %s:\n%s\n", L_WARNxSTR,
           m_szPathName, p.ErrorStr());
        return false;
      }
      if (!SendPacket(&p)) return false;

      m_nFileSize = p.GetFileSize();
      strcpy(m_szFileName, p.GetFileName());

      m_nBatchStartTime = time(TIME_NOW);
      m_nBatchBytesTransfered = m_nBatchPos = 0;

      m_nState = FT_STATE_WAITxFORxSTART;
      break;
    }

    case FT_STATE_WAITxFORxSTART:
    {
      // contains the seek value
      char nCmd = b.UnpackChar();
      if (nCmd == 0x05)
      {
        // set our speed, for now fuckem and go as fast as possible
        break;
      }
      if (nCmd != 0x03)
      {
        char *pbuf;
        gLog.Error("%sFile Transfer: Invalid start packet:\n%s%s\n",
                   L_ERRORxSTR, L_BLANKxSTR, b.print(pbuf));
        delete [] pbuf;
        return false;
      }

      m_nBytesTransfered = 0;
      m_nCurrentFile++;

      m_nFilePos = b.UnpackUnsignedLong();

      m_nFileDesc = open(m_szPathName, O_RDONLY);
      if (m_nFileDesc < 0)
      {
        gLog.Error("%sFile Transfer: Read error '%s':\n%s%s\n.", L_ERRORxSTR,
           m_szPathName, L_BLANKxSTR, strerror(errno));
        return false;
      }

      if (lseek(m_nFileDesc, m_nFilePos, SEEK_SET) < 0)
      {
        gLog.Error("%sFile Transfer: Seek error '%s':\n%s%s\n.", L_ERRORxSTR,
                   m_szFileName, L_BLANKxSTR, strerror(errno));
        return false;
      }

      m_nState = FT_STATE_SENDINGxFILE;
      break;
    }

    case FT_STATE_SENDINGxFILE:
    {
      char nCmd = b.UnpackChar();
      if (nCmd == 0x05)
      {
        break;
      }
      char *p;
      gLog.Unknown("%sFile Transfer: Unknown packet received during file send:\n%s\n",
         L_UNKNOWNxSTR, b.print(p));
      delete [] p;
      break;
    }


    default:
    {
      gLog.Error("%sInternal error: FileTransferManager::ProcessPacket(), invalid state (%d).\n",
         L_ERRORxSTR, m_nState);
      break;
    }

  } // switch

  ftSock.ClearRecvBuffer();

  return true;
}


//-----CFileTransferManager::SendFilePacket----------------------------------
bool CFileTransferManager::SendFilePacket()
{
  static char pSendBuf[2048];

  if (m_nBytesTransfered == 0)
  {
    m_nStartTime = time(TIME_NOW);
    m_nBatchPos += m_nFilePos;
    gLog.Info("%sFile Transfer: Sending %s (%ld bytes).\n", L_TCPxSTR,
       m_szFileName, m_nFileSize);
  }

  int nBytesToSend = m_nFileSize - m_nFilePos;
  if (nBytesToSend > 2048) nBytesToSend = 2048;
  if (read(m_nFileDesc, pSendBuf, nBytesToSend) != nBytesToSend)
  {
    gLog.Error("%sFile Transfer: Error reading from %s:\n%s%s.\n", L_ERRORxSTR,
       m_szFileName, L_BLANKxSTR, strerror(errno));
    return false;
  }
  CBuffer xSendBuf(nBytesToSend + 1);
  xSendBuf.PackChar(0x06);
  xSendBuf.Pack(pSendBuf, nBytesToSend);
  if (!SendBuffer(&xSendBuf)) return false;

  m_nFilePos += nBytesToSend;
  m_nBytesTransfered += nBytesToSend;

  m_nBatchPos += nBytesToSend;
  m_nBatchBytesTransfered += nBytesToSend;

  int nBytesLeft = m_nFileSize - m_nFilePos;
  if (nBytesLeft > 0)
  {
    // More bytes to send so go away until the socket is free again
    return true;
  }

  // Only get here if we are done
  close(m_nFileDesc);
  m_nFileDesc = -1;

  if (nBytesLeft == 0)
  {
    gLog.Info("%sFile Transfer: Sent %s.\n", L_TCPxSTR, m_szFileName);
  }
  else // nBytesLeft < 0
  {
    gLog.Info("%sFile Transfer: Sent %s, %d too many bytes.\n", L_TCPxSTR,
       m_szFileName, -nBytesLeft);
  }

  // FIXME go to the next file, if no more then close connections
  // send plugin notification in either case?
  CloseConnection();
  //m_nState = FT_STATE_WAITxFORxSTART;

  return true;
}



//-----CFileTransferManager::PopChatEvent------------------------------------
CFileTransferEvent *CFileTransferManager::PopFileTransferEvent()
{
  if (ftEvents.size() == 0) return NULL;

  CFileTransferEvent *e = ftEvents.front();
  ftEvents.pop_front();

  return e;
}


//-----CFileTransferManager::PushChatEvent-------------------------------------------
void CFileTransferManager::PushFileTransferEvent(CFileTransferEvent *e)
{
  ftEvents.push_back(e);
  write(pipe_events[PIPE_WRITE], "*", 1);
}


//-----CFileTransferManager::SendPacket----------------------------------------------
bool CFileTransferManager::SendPacket(CPacket *p)
{
  return SendBuffer(p->getBuffer());
}


//-----CFileTransferManager::SendBuffer----------------------------------------------
bool CFileTransferManager::SendBuffer(CBuffer *b)
{
  if (!ftSock.SendPacket(b))
  {
    char buf[128];
    gLog.Warn("%sFile Transfer: Send error:\n%s%s\n", L_WARNxSTR, L_BLANKxSTR,
       ftSock.ErrorStr(buf, 128));
    return false;
  }
  return true;
}


void CFileTransferManager::ChangeSpeed(unsigned short nSpeed)
{
  if (nSpeed > 100)
  {
    gLog.Warn("%sInvalid file transfer speed: %d%%.\n", L_WARNxSTR, nSpeed);
    return;
  }

  //CPFile_ChangeSpeed p(nSpeed);
  //SendPacket(&p);
  m_nSpeed = nSpeed;
}



//----CFileTransferManager::CloseFileTransfer--------------------------------
void CFileTransferManager::CloseFileTransfer()
{
  // Close the thread
  if (pipe_thread[PIPE_WRITE] != -1)
  {
    write(pipe_thread[PIPE_WRITE], "X", 1);
    pthread_join(thread_ft, NULL);

    close(pipe_thread[PIPE_READ]);
    close(pipe_thread[PIPE_WRITE]);

    pipe_thread[PIPE_READ] = pipe_thread[PIPE_WRITE] = -1;
  }

  CloseConnection();
}


//----CFileTransferManager::CloseConnection----------------------------------
void CFileTransferManager::CloseConnection()
{
  sockman.CloseSocket(ftServer.Descriptor(), false, false);
  sockman.CloseSocket(ftSock.Descriptor(), false, false);
  m_nState = FT_STATE_DISCONNECTED;
}



void *FileTransferManager_tep(void *arg)
{
  CFileTransferManager *ftman = (CFileTransferManager *)arg;

  licq_segv_handler(&signal_handler_ftThread);

  fd_set f_recv, f_send;
  int l, nSocketsAvailable, nCurrentSocket;
  char buf[2];

  while (true)
  {
    f_recv = ftman->sockman.SocketSet();
    l = ftman->sockman.LargestSocket() + 1;

    // Add the new socket pipe descriptor
    FD_SET(ftman->pipe_thread[PIPE_READ], &f_recv);
    if (ftman->pipe_thread[PIPE_READ] >= l)
      l = ftman->pipe_thread[PIPE_READ] + 1;

    // Set up the send descriptor
    FD_ZERO(&f_send);
    if (ftman->m_nState == FT_STATE_SENDINGxFILE)
    {
      FD_SET(ftman->ftSock.Descriptor(), &f_send);
      // No need to check "l" as ftSock is already in the read list
    }

    nSocketsAvailable = select(l, &f_recv, &f_send, NULL, NULL);

    nCurrentSocket = 0;
    while (nSocketsAvailable > 0 && nCurrentSocket < l)
    {
      if (FD_ISSET(nCurrentSocket, &f_recv))
      {
        // New socket event ----------------------------------------------------
        if (nCurrentSocket == ftman->pipe_thread[PIPE_READ])
        {
          read(ftman->pipe_thread[PIPE_READ], buf, 1);
          if (buf[0] == 'S')
          {
            DEBUG_THREADS("[FileTransferManager_tep] Reloading socket info.\n");
          }
          else if (buf[0] == 'X')
          {
            DEBUG_THREADS("[FileTransferManager_tep] Exiting.\n");
            pthread_exit(NULL);
          }
        }

        // Connection on the server port ---------------------------------------
        else if (nCurrentSocket == ftman->ftServer.Descriptor())
        {
          if (ftman->ftSock.Descriptor() != -1)
          {
            gLog.Warn("%sFile Transfer: Receiving repeat incoming connection.\n", L_WARNxSTR);
          }
          else
          {
            ftman->ftServer.RecvConnection(ftman->ftSock);
            ftman->sockman.AddSocket(&ftman->ftSock);
            ftman->sockman.DropSocket(&ftman->ftSock);

            ftman->m_nState = FT_STATE_HANDSHAKE;
            gLog.Info("%sFile Transfer: Received connection.\n", L_TCPxSTR);
          }
        }

        // Message from connected socket----------------------------------------
        else if (nCurrentSocket == ftman->ftSock.Descriptor())
        {
          ftman->ftSock.Lock();
          bool ok = ftman->ProcessPacket();
          ftman->ftSock.Unlock();
          if (!ok)
          {
            ftman->CloseConnection();
            //FIXME notify plugin
          }
        }

        else
        {
          gLog.Warn("%sFile Transfer: No such socket.\n", L_WARNxSTR);
        }

        nSocketsAvailable--;
      }
      else if (FD_ISSET(nCurrentSocket, &f_send))
      {
        if (nCurrentSocket == ftman->ftSock.Descriptor())
        {
          ftman->ftSock.Lock();
          bool ok = ftman->SendFilePacket();
          ftman->ftSock.Unlock();
          if (!ok)
          {
            ftman->CloseConnection();
            // FIXME notify plugin
          }
        }
        nSocketsAvailable--;
      }

      nCurrentSocket++;
    }
  }
  return NULL;
}



CFileTransferManager::~CFileTransferManager()
{
  CloseFileTransfer();

  // Delete any pending events
  CFileTransferEvent *e = NULL;
  while (ftEvents.size() > 0)
  {
    e = ftEvents.front();
    delete e;
    ftEvents.pop_front();
  }
}

