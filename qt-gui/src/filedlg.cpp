#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef USE_KDE
#include <kfiledialog.h>
#include <qfiledialog.h>
#else
#include <qfiledialog.h>
#endif
#include <qgroupbox.h>

#include "filedlg.h"
#include "icqpacket.h"
#include "log.h"
#include "translate.h"
#include "ewidgets.h"
#include "support.h"
#include "licq-locale.h"

extern int errno;

// State table defines
#define STATE_RECVxHANDSHAKE 1
#define STATE_RECVxCLIENTxINIT 2
#define STATE_RECVxFILExINFO 3
#define STATE_RECVxFILE 4
#define STATE_RECVxSERVERxINIT 5
#define STATE_RECVxSTART 6
#define STATE_SENDxFILE 7


//-----Constructor--------------------------------------------------------------
CFileDlg::CFileDlg(unsigned long _nUin,
                   const char *_szTransferFileName, unsigned long _nFileSize,
                   bool _bServer, unsigned short _nPort,
                   QWidget *parent = NULL, char *name = NULL)
   : QWidget(parent, name)
{
   // If we are the server, then we are receiving a file
   char t[64];
   m_nUin = _nUin;
   m_nPort = _nPort;
   m_bServer = _bServer;
   m_nFileSize = _nFileSize;
   m_nCurrentFile = 0;
   m_nFileDesc = 0;
   m_nBatchSize = m_nFileSize;
   m_nTotalFiles = 1;
   ICQOwner *o = gUserManager.FetchOwner(LOCK_R);
   m_szLocalName = strdup(o->getAlias());
   gUserManager.DropOwner();
   m_szRemoteName = NULL;
   m_snSend = snFile = snFileServer = NULL;

   setCaption(_("ICQ file transfer"));
   setGeometry(100, 100, 500, 325);

   nfoTransferFileName = new CInfoField(10, 15, 100, 5, 290, _("Current file:"), true, this);
   nfoTransferFileName->setData(_szTransferFileName);
   nfoTotalFiles = new CInfoField(410, 15, 0, 0, 60, NULL, true, this);

   nfoLocalFileName = new CInfoField(10, 40, 100, 5, 355, _("Local file name:"), true, this);
   nfoLocalFileName->setData(IsServer() ? _("Unset") : _("N/A"));
   nfoLocalFileName->setEnabled(IsServer());

   // Information stuff about the current file
   QGroupBox *boxCurrent = new QGroupBox(_("Current File"), this);
   boxCurrent->setGeometry(10, 80, 220, 170);
   nfoFileSize = new CInfoField(10, 15, 35, 5, 150, _("Size:"), true, boxCurrent);
   sprintf(t, "%ld bytes", m_nFileSize);
   nfoFileSize->setData(t);
   nfoTrans = new CInfoField(10, 40, 35, 5, 150, _("Trans:"), true, boxCurrent);
   nfoTime = new CInfoField(10, 65, 35, 5, 150, _("Time:"), true, boxCurrent);
   nfoBPS = new CInfoField(10, 90, 35, 5, 150, _("BPS:"), true, boxCurrent);
   nfoETA = new CInfoField(10, 115, 35, 5, 150, _("ETA:"), true, boxCurrent);
   barTransfer = new QProgressBar(boxCurrent);
   barTransfer->setGeometry(10, 140, 200, 20);

   QGroupBox *boxBatch = new QGroupBox("Batch", this);
   boxBatch->setGeometry(250, 80, 220, 170);
   nfoBatchSize = new CInfoField(10, 15, 35, 5, 150, _("Size:"), true, boxBatch);
   nfoBatchTrans = new CInfoField(10, 40, 35, 5, 150, _("Trans:"), true, boxBatch);
   nfoBatchTime = new CInfoField(10, 65, 35, 5, 150, _("Time:"), true, boxBatch);
   nfoBatchBPS = new CInfoField(10, 90, 35, 5, 150, _("BPS:"), true, boxBatch);
   nfoBatchETA = new CInfoField(10, 115, 35, 5, 150, _("ETA:"), true, boxBatch);
   barBatchTransfer = new QProgressBar(boxBatch);
   barBatchTransfer->setGeometry(10, 140, 200, 20);

   lblStatus = new QLabel(this);
   lblStatus->setFrameStyle(QFrame::Box | QFrame::Raised);

   btnCancel = new QPushButton(_("&Cancel Transfer"), this);
   connect(btnCancel, SIGNAL(clicked()), this, SLOT(hide()));
   connect(&m_tUpdate, SIGNAL(timeout()), this, SLOT(fileUpdate()));

   // now either connect to the remote host or start up a server
   if (IsServer())
   {
      sprintf(t, "%d / ?", m_nCurrentFile + 1);
      nfoTotalFiles->setData(t);
      nfoBatchSize->setData(_("Unknown"));
      show();
      if (!startAsServer()) setPort(0);
   }
   else
   {
      sprintf(t, "%d / %ld", m_nCurrentFile + 1, m_nTotalFiles);
      nfoTotalFiles->setData(t);
      sprintf(t, _("%ld bytes"), m_nBatchSize);
      nfoBatchSize->setData(t);
      show();
      if (!startAsClient()) setPort(0);
   }
}


//-----Destructor---------------------------------------------------------------
CFileDlg::~CFileDlg(void)
{
  if (m_szLocalName != NULL) free(m_szLocalName);
  if (m_szRemoteName != NULL) delete[] m_szRemoteName;
  if (m_nFileDesc > 0) ::close(m_nFileDesc);
  if (m_snSend != NULL) delete m_snSend;
  if (snFile != NULL) delete snFile;
  if (snFileServer != NULL) delete snFileServer;
}


//-----GetLocalFileName---------------------------------------------------------
// Sets the local filename and opens the file
// returns false if the user hits cancel
bool CFileDlg::GetLocalFileName(void)
{
  QString f;
  bool bValid = false;
  // Get the local filename and open it, loop until valid or cancel
  while(!bValid)
  {
#ifdef USE_KDE
    f = QFileDialog::getSaveFileName(m_sFileInfo.szName, QString::null, this);
#else
    f = QFileDialog::getSaveFileName(m_sFileInfo.szName, QString::null, this);
#endif
    if (f.isNull()) return (false);
    struct stat buf;
    int nFlags = O_WRONLY;
    m_nFilePos = 0;
    if (stat(f, &buf) == 0)  // file already exists
    {
      if ((unsigned long)buf.st_size >= m_sFileInfo.nSize)
      {
        if(QueryUser(this, _("File already exists and is at least as big as the incoming file."), _("Overwrite"), _("Cancel")))
          nFlags |= O_TRUNC;
        else
          return (false);
      }
      else
      {
        if (QueryUser(this, _("File already exists and appears incomplete."), _("Overwrite"), _("Resume")))
          nFlags |= O_TRUNC;
        else
        {
          nFlags |= O_APPEND;
          m_nFilePos = buf.st_size;
        }
      }
    }
    else
    {
      nFlags |= O_CREAT;
    }

    m_nFileDesc = open (f, nFlags, 00664);
    if (m_nFileDesc < 0)
    {
      if (!QueryUser(this, _("Open error - unable to open file for writing."), _("Retry"), _("Cancel")))
        return (false);
    }
    else
      bValid = true;
  }

  nfoLocalFileName->setData(f);
  m_nBytesTransfered = 0;
  barTransfer->setTotalSteps(m_sFileInfo.nSize);
  barTransfer->setProgress(0);
  return(true);
}


//-----fileCancel---------------------------------------------------------------
void CFileDlg::fileCancel()
{
  // close the local file and other stuff
  if (m_snSend != NULL) m_snSend->setEnabled(false);
  if (snFile != NULL) snFile->setEnabled(false);
  if (snFileServer != NULL) snFileServer->setEnabled(false);
  m_xSocketFileServer.CloseConnection();
  m_xSocketFile.CloseConnection();
  lblStatus->setText(_("File transfer cancelled."));
  btnCancel->setText(_("Done"));
}


//-----fileUpdate---------------------------------------------------------------
void CFileDlg::fileUpdate()
{
  //update time, BPS and eta
  static char sz[16];

  // Current File

  // Transfered
  if (m_nFilePos > 1024)
    sprintf(sz, _("%0.2f kb"), m_nFilePos / 1024.0);
  else
    sprintf(sz, _("%ld b"), m_nFilePos);
  nfoTrans->setData(sz);

  // Time
  time_t nTime = time(NULL) - m_nStartTime;
  sprintf(sz, "%02ld:%02ld:%02ld", nTime / 3600, (nTime % 3600) / 60, (nTime % 60));
  nfoTime->setData(sz);
  if (nTime == 0 || m_nBytesTransfered == 0)
  {
    nfoBPS->setData("---");
    nfoETA->setData("---");
    return;
  }

  // BPS
  float fBPS = m_nBytesTransfered / nTime;
  if (fBPS > 1024)
    sprintf(sz, _("%.2f k"), fBPS / 1024);
  else
    sprintf(sz, "%.2f", fBPS);
  nfoBPS->setData(sz);

  // ETA
  int nBytesLeft = m_sFileInfo.nSize - m_nFilePos;
  time_t nETA = (time_t)(nBytesLeft / fBPS);
  sprintf(sz, "%02ld:%02ld:%02ld", nETA / 3600, (nETA % 3600) / 60, (nETA % 60));
  nfoETA->setData(sz);


  // Batch

  // Transfered
  if (m_nBatchPos > 1024)
    sprintf(sz, _("%0.2f kb"), m_nBatchPos / 1024.0);
  else
    sprintf(sz, _("%ld b"), m_nBatchPos);
  nfoBatchTrans->setData(sz);

  // Time
  time_t nBatchTime = time(NULL) - m_nBatchStartTime;
  sprintf(sz, "%02ld:%02ld:%02ld", nBatchTime / 3600, (nBatchTime % 3600) / 60,
         (nBatchTime % 60));
  nfoBatchTime->setData(sz);
  if (nBatchTime == 0 || m_nBatchBytesTransfered == 0)
  {
    nfoBatchBPS->setData("---");
    nfoBatchETA->setData("---");
    return;
  }

  // BPS
  float fBatchBPS = m_nBatchBytesTransfered / nBatchTime;
  if (fBatchBPS > 1024)
    sprintf(sz, _("%.2f k"), fBatchBPS / 1024);
  else
    sprintf(sz, "%.2f", fBatchBPS);
  nfoBatchBPS->setData(sz);

  // ETA
  int nBatchBytesLeft = m_nBatchSize - m_nBatchPos;
  time_t nBatchETA = (time_t)(nBatchBytesLeft / fBatchBPS);
  sprintf(sz, "%02ld:%02ld:%02ld", nBatchETA / 3600, (nBatchETA % 3600) / 60,
          (nBatchETA % 60));
  nfoBatchETA->setData(sz);

}



//=====Server Side==============================================================

//-----startAsServer------------------------------------------------------------
bool CFileDlg::startAsServer(void)
{
   gLog.Info("%sStarting file server on port %d.\n", L_TCPxSTR, getPort());
   if (!(m_xSocketFileServer.StartServer(getPort())))
   {
     gLog.Error("%sFile transfer - error creating local socket:\n%s%s\n", L_ERRORxSTR,
                L_BLANKxSTR, m_xSocketFileServer.ErrorStr(buf, 128));
     return false;
   }

   setPort(m_xSocketFileServer.LocalPort());
   snFileServer = new QSocketNotifier(m_xSocketFileServer.Descriptor(), QSocketNotifier::Read);
   connect(snFileServer, SIGNAL(activated(int)), this, SLOT(fileRecvConnection()));

   lblStatus->setText(_("Waiting for connection..."));

   return true;
}


//-----fileRecvConnection-------------------------------------------------------
void CFileDlg::fileRecvConnection()
{
   m_xSocketFileServer.RecvConnection(m_xSocketFile);
   disconnect(snFileServer, SIGNAL(activated(int)), this, SLOT(fileRecvConnection()));
   m_nState = STATE_RECVxHANDSHAKE;
   snFile = new QSocketNotifier(m_xSocketFile.Descriptor(), QSocketNotifier::Read);
   connect(snFile, SIGNAL(activated(int)), this, SLOT(StateServer()));
}


//----StateServer----------------------------------------------------------------
void CFileDlg::StateServer()
{
  // get the handshake packet
  if (!m_xSocketFile.RecvPacket())
  {
    fileCancel();
    if (m_xSocketFile.Error() == 0)
      InformUser(this, _("Remote end disconnected."));
    else
      gLog.Error("%sFile transfer receive error - lost remote end:\n%s%s\n", L_ERRORxSTR,
                 L_BLANKxSTR, m_xSocketFile.ErrorStr(buf, 128));
    return;
  }
  if (!m_xSocketFile.RecvBufferFull()) return;

  switch (m_nState)
  {
  case STATE_RECVxHANDSHAKE:
  {
    char cHandshake;
    m_xSocketFile.RecvBuffer() >> cHandshake;
    if ((unsigned short)cHandshake != ICQ_CMDxTCP_HANDSHAKE)
    {
      gLog.Error("%sReceive error - bad handshake (%04X).\n", L_ERRORxSTR, cHandshake);
      fileCancel();
      return;
    }
    m_nState = STATE_RECVxCLIENTxINIT;
    break;
  }

  case STATE_RECVxCLIENTxINIT:
  {
    // Process init packet
    char cJunk, t[64];
    unsigned long nJunkLong;
    unsigned short nRemoteNameLen;
    m_xSocketFile.RecvBuffer() >> cJunk;
    if (cJunk != 0x00)
    {
      char *pbuf;
      gLog.Error("%sError receiving data: invalid client init packet:\n%s%s\n",
                 L_ERRORxSTR, L_BLANKxSTR, m_xSocketFile.RecvBuffer().print(pbuf));
      delete [] pbuf;
      fileCancel();
      return;
    }
    m_xSocketFile.RecvBuffer() >> nJunkLong
                               >> m_nTotalFiles
                               >> m_nBatchSize
                               >> nJunkLong
                               >> nRemoteNameLen;
    m_szRemoteName = new char[nRemoteNameLen];
    for (int i = 0; i < nRemoteNameLen; i++)
       m_xSocketFile.RecvBuffer() >> m_szRemoteName[i];
    sprintf(t, "%d / %ld", m_nCurrentFile + 1, m_nTotalFiles);
    nfoTotalFiles->setData(t);
    sprintf(t, "%ld bytes", m_nBatchSize);
    nfoBatchSize->setData(t);
    m_nBatchStartTime = time(NULL);
    m_nBatchBytesTransfered = m_nBatchPos = 0;
    barBatchTransfer->setTotalSteps(m_nBatchSize);
    barBatchTransfer->setProgress(0);
    sprintf(t, _("ICQ file transfer %s %s"), IsServer() ? _("from") : _("to"), m_szRemoteName);
    setCaption(t);

    // Send response
    CPFile_InitServer p(m_szLocalName);
    m_xSocketFile.SendPacket(p.getBuffer());

    lblStatus->setText(_("Received init, waiting for batch info..."));
    m_nState = STATE_RECVxFILExINFO;
    break;
  }

  case STATE_RECVxFILExINFO:
  {
    // Process file packet
    unsigned short nLen, nTest;
    char t[64], cJunk;
    m_xSocketFile.RecvBuffer() >> nTest;
    if (nTest != 0x0002)
    {
      char *pbuf;
      gLog.Error("%sError receiving data: invalid file info packet:\n%s%s\n",
                 L_ERRORxSTR, L_BLANKxSTR, m_xSocketFile.RecvBuffer().print(pbuf));
      delete [] pbuf;
      fileCancel();
      return;
    }
    m_xSocketFile.RecvBuffer() >> nLen;
    for (int j = 0; j < nLen; j++)
      m_xSocketFile.RecvBuffer() >> m_sFileInfo.szName[j];
    m_xSocketFile.RecvBuffer() >> nLen;
    m_xSocketFile.RecvBuffer() >> cJunk;
    m_xSocketFile.RecvBuffer() >> m_sFileInfo.nSize;

    m_nCurrentFile++;
    nfoTransferFileName->setData(m_sFileInfo.szName);
    sprintf(t, _("%ld bytes"), m_sFileInfo.nSize);
    nfoFileSize->setData(t);
    barTransfer->setTotalSteps(m_sFileInfo.nSize);

    // Get the local filename and set the file offset (for resume)
    if (!GetLocalFileName())
    {
      fileCancel();
      return;
    }

    // Send response
    CPFile_Start p(m_nFilePos);
    m_xSocketFile.SendPacket(p.getBuffer());
    lblStatus->setText("Starting transfer...");

    // Update the status every 2 seconds
    m_tUpdate.start(2000);

    disconnect(snFile, SIGNAL(activated(int)), this, SLOT(StateServer()));
    connect(snFile, SIGNAL(activated(int)), this, SLOT(fileRecvFile()));
    m_nState = STATE_RECVxFILE;
    break;
  }

  }  // switch

  m_xSocketFile.ClearRecvBuffer();
}


//-----fileRecvFile-------------------------------------------------------------
void CFileDlg::fileRecvFile()
{
  if (!m_xSocketFile.RecvPacket())
  {
    fileCancel();
    if (m_xSocketFile.Error() == 0)
      gLog.Error("%sFile receive error, remote end disconnected.\n", L_ERRORxSTR);
    else
      gLog.Error("%sFile receive error:\n%s%s\n", L_ERRORxSTR, L_BLANKxSTR,
                 m_xSocketFile.ErrorStr(buf, 128));
    return;
  }
  if (!m_xSocketFile.RecvBufferFull()) return;

  // if this is the first call to this function...
  if (m_nBytesTransfered == 0)
  {
    m_nStartTime = time(NULL);
    m_nBatchPos += m_nFilePos;
    lblStatus->setText(_("Receiving file..."));
  }

  // Write the new data to the file and empty the buffer
  CBuffer &b = m_xSocketFile.RecvBuffer();
  char cTest;
  b >> cTest;
  if (cTest != 0x06)
  {
    gLog.Error("%sFile receive error, invalid data (%c).  Ignoring packet.\n", cTest);
               //, L_BLANKxSTR, m_xSocketFile.RecvBuffer().print());
    m_xSocketFile.ClearRecvBuffer();
    return;
  }
  errno = 0;
  size_t nBytesWritten = write(m_nFileDesc, b.getDataPosRead(), b.getDataSize() - 1);
  if (nBytesWritten != b.getDataSize() - 1)
  {
    gLog.Error("%sFile write error:\n%s%s.\n", L_ERRORxSTR, L_BLANKxSTR,
               errno == 0 ? "Disk full (?)" : strerror(errno));
    fileCancel();
    return;
  }

  m_nFilePos += nBytesWritten;
  m_nBytesTransfered += nBytesWritten;
  barTransfer->setProgress(m_nFilePos);

  m_nBatchPos += nBytesWritten;
  m_nBatchBytesTransfered += nBytesWritten;
  barBatchTransfer->setProgress(m_nBatchPos);

  m_xSocketFile.ClearRecvBuffer();

  int nBytesLeft = m_sFileInfo.nSize - m_nFilePos;
  if (nBytesLeft > 0)
  {
    // More bytes to come so go away and wait for them
    return;
  }
  else if (nBytesLeft == 0)
  {
    // File transfer done perfectly
    ::close(m_nFileDesc);
    m_nFileDesc = 0;
    char msg[1024];
    sprintf(msg, _("%sFile transfer of\n'%s'\nfrom %s completed successfully.\n"),
            L_TCPxSTR, m_sFileInfo.szName, m_szRemoteName);
    InformUser(this, msg);
  }
  else // nBytesLeft < 0
  {
    // Received too many bytes for the given size of the current file
    ::close(m_nFileDesc);
    m_nFileDesc = 0;
    gLog.Error("%sFile transfer of\n'%s'\nfrom %s received %d too many bytes.\n%sClosing file, recommend check for errors.\n",
               L_TCPxSTR, m_sFileInfo.szName, m_szRemoteName, -nBytesLeft, L_BLANKxSTR);
  }
  // Only get here if the current file is done
  m_nCurrentFile++;
  m_tUpdate.stop();
  btnCancel->setText(_("Ok"));
  lblStatus->setText(_("File received."));
  disconnect(snFile, SIGNAL(activated(int)), this, SLOT(fileRecvFile()));

  // Now wait for a disconnect or another file
  m_nState = STATE_RECVxFILExINFO;
  connect(snFile, SIGNAL(activated(int)), this, SLOT(StateServer()));
}



//=====Client Side==============================================================

//-----startAsClient------------------------------------------------------------
bool CFileDlg::startAsClient(void)
{
  ICQUser *u = gUserManager.FetchUser(m_nUin, LOCK_R);
  unsigned long nIp = u->Ip();
  gUserManager.DropUser(u);
  gLog.Info("%sFile transfer - connecting to %ld:%d.\n", L_TCPxSTR,
             inet_ntoa_r(*(struct in_addr *)&nIp, buf), getPort());
   lblStatus->setText(_("Connecting to remote..."));
   m_xSocketFile.SetRemoteAddr(nIp, getPort());
   if (!m_xSocketFile.OpenConnection())
   {
     gLog.Error("%sUnable to connect to remote file server:\n%s%s.\n", L_ERRORxSTR,
                L_BLANKxSTR, m_xSocketFile.ErrorStr(buf, 128));
     return false;
   }

   lblStatus->setText(_("Connected, shaking hands..."));

   // Send handshake packet:
   CPacketTcp_Handshake p_handshake(getLocalPort());
   m_xSocketFile.SendPacket(p_handshake.getBuffer());

   // Send init packet:
   CPFile_InitClient p(m_szLocalName, 1, m_nFileSize);
   m_xSocketFile.SendPacket(p.getBuffer());

   lblStatus->setText(_("Connected, waiting for response..."));
   m_nState = STATE_RECVxSERVERxINIT;
   snFile = new QSocketNotifier(m_xSocketFile.Descriptor(), QSocketNotifier::Read);
   connect(snFile, SIGNAL(activated(int)), this, SLOT(StateClient()));

   return true;
}

//-----StateClient--------------------------------------------------------------
void CFileDlg::StateClient()
{
  if (!m_xSocketFile.RecvPacket())
  {
    fileCancel();
    if (m_xSocketFile.Error() == 0)
      InformUser(this, _("Remote end disconnected."));
    else
      gLog.Error("%sFile transfer receive error - lost remote end:\n%s%s\n", L_ERRORxSTR,
                L_BLANKxSTR, m_xSocketFile.ErrorStr(buf, 128));
    return;
  }
  if (!m_xSocketFile.RecvBufferFull()) return;

  switch(m_nState)
  {
  case STATE_RECVxSERVERxINIT:
  {
    // Process init packet
    char cJunk, t[64];
    unsigned long nJunkLong;
    unsigned short nRemoteNameLen;
    m_xSocketFile.RecvBuffer() >> cJunk >> nJunkLong >> nRemoteNameLen;
    // Cheap hack to avoid icq99a screwing us up, the next packet should good
    if (cJunk == 0x05)
    {
      // set our speed, for now fuckem and go as fast as possible
      break;
    }
    if (cJunk != 0x01)
    {
      char *pbuf;
      gLog.Error("%sError receiving data: invalid server init packet:\n%s%s\n",
                 L_ERRORxSTR, L_BLANKxSTR, m_xSocketFile.RecvBuffer().print(pbuf));
      delete [] pbuf;
      fileCancel();
      return;
    }
    m_szRemoteName = new char[nRemoteNameLen];
    for (int i = 0; i < nRemoteNameLen; i++)
       m_xSocketFile.RecvBuffer() >> m_szRemoteName[i];
    sprintf(t, _("ICQ file transfer %s %s"), IsServer() ? _("from") : _("to"), m_szRemoteName);
    setCaption(t);

    // Send file info packet
    CPFile_Info p(nfoTransferFileName->text());
    if (!p.IsValid())
    {
      gLog.Error("%sFile read error '%s':\n%s%s\n.", L_ERRORxSTR, (const char *)nfoTransferFileName->text(),
                 L_BLANKxSTR, p.ErrorStr());
      fileCancel();
      return;
    }
    m_xSocketFile.SendPacket(p.getBuffer());
    lblStatus->setText(_("Sent batch info, waiting for ack..."));

    // Set up local batch info
    strcpy(m_sFileInfo.szName, nfoTransferFileName->text());
    m_sFileInfo.nSize = p.GetFileSize();
    m_nCurrentFile++;

    m_nBatchStartTime = time(NULL);
    m_nBatchBytesTransfered = m_nBatchPos = 0;
    barBatchTransfer->setTotalSteps(m_nBatchSize);
    barBatchTransfer->setProgress(0);

    m_nState = STATE_RECVxSTART;
    break;
  }

  case STATE_RECVxSTART:
  {
    // Process batch ack packet, it contains nothing useful so just start the transfer
    lblStatus->setText(_("Starting transfer..."));

    // contains the seek value
    char cJunk;
    m_xSocketFile.RecvBuffer() >> cJunk >> m_nFilePos;
    if (cJunk != 0x03)
    {
      char *pbuf;
      gLog.Error("%sError receiving data: invalid start packet:\n%s%s\n",
                 L_ERRORxSTR, L_BLANKxSTR, m_xSocketFile.RecvBuffer().print(pbuf));
      delete [] pbuf;
      fileCancel();
      return;
    }

    m_nFileDesc = open(m_sFileInfo.szName, O_RDONLY);
    if (m_nFileDesc < 0)
    {
      gLog.Error("%sFile read error '%s':\n%s%s\n.", L_ERRORxSTR,
                 m_sFileInfo.szName, L_BLANKxSTR, strerror(errno));
      fileCancel();
      return;
    }

    if (lseek(m_nFileDesc, m_nFilePos, SEEK_SET) < 0)
    {
      gLog.Error("%sFile seek error '%s':\n%s%s\n.", L_ERRORxSTR,
                 m_sFileInfo.szName, L_BLANKxSTR, strerror(errno));
      fileCancel();
      return;
    }

    m_snSend = new QSocketNotifier(m_xSocketFile.Descriptor(), QSocketNotifier::Write);
    connect(m_snSend, SIGNAL(activated(int)), this, SLOT(fileSendFile()));

    m_nBytesTransfered = 0;
    barTransfer->setTotalSteps(m_sFileInfo.nSize);
    barTransfer->setProgress(0);

    // Update the status every 2 seconds
    m_tUpdate.start(2000);

    m_nState = STATE_SENDxFILE;
    break;
  }

  case STATE_SENDxFILE:
    // I don't know what this would be...
    break;

  } // switch

  m_xSocketFile.ClearRecvBuffer();
}


//-----fileSendFile-------------------------------------------------------------
void CFileDlg::fileSendFile()
{
  static char pSendBuf[2048];

  if (m_nBytesTransfered == 0)
  {
    m_nStartTime = time(NULL);
    m_nBatchPos += m_nFilePos;
    lblStatus->setText(_("Sending file..."));
  }

  int nBytesToSend = m_sFileInfo.nSize - m_nFilePos;
  if (nBytesToSend > 2048) nBytesToSend = 2048;
  if (read(m_nFileDesc, pSendBuf, nBytesToSend) != nBytesToSend)
  {
    gLog.Error("%sError reading from %s:\n%s%s.\n", L_ERRORxSTR,
               m_sFileInfo.szName, L_BLANKxSTR, strerror(errno));
    fileCancel();
    return;
  }
  CBuffer xSendBuf(nBytesToSend + 1);
  xSendBuf.add((char)0x06);
  xSendBuf.add(pSendBuf, nBytesToSend);
  if (!m_xSocketFile.SendPacket(&xSendBuf))
  {
    gLog.Error("%sFile send error:\n%s%s\n", L_ERRORxSTR, L_BLANKxSTR,
               m_xSocketFile.ErrorStr(buf, 128));
    fileCancel();
    return;
  }

  m_nFilePos += nBytesToSend;
  m_nBytesTransfered += nBytesToSend;
  barTransfer->setProgress(m_nFilePos);

  m_nBatchPos += nBytesToSend;
  m_nBatchBytesTransfered += nBytesToSend;
  barBatchTransfer->setProgress(m_nBatchPos);

  int nBytesLeft = m_sFileInfo.nSize - m_nFilePos;
  if (nBytesLeft > 0)
  {
    // More bytes to send so go away until the socket is free again
    return;
  }

  // Only get here if we are done
  delete m_snSend;
  m_snSend = NULL;
  ::close(m_nFileDesc);
  m_nFileDesc = 0;
  m_tUpdate.stop();
  btnCancel->setText(_("Ok"));
  lblStatus->setText(_("File sent."));

  if (nBytesLeft == 0)
  {
    // File transfer done perfectly
    char msg[1024];
    sprintf(msg, _("%sFile transfer of\n'%s'\nto %s completed successfully.\n"),
            L_TCPxSTR, m_sFileInfo.szName, m_szRemoteName);
    InformUser(this, msg);
  }
  else // nBytesLeft < 0
  {
    // Sent too many bytes for the given size of the current file, can't really happen
    gLog.Error("%sFile transfer of\n'%s'\n to %s received %d too many bytes.\n%sClosing file, recommend check for errors.\n", 
               L_TCPxSTR, m_sFileInfo.szName, m_szRemoteName, -nBytesLeft,
               L_BLANKxSTR);
  }


  m_xSocketFileServer.CloseConnection();
  m_xSocketFile.CloseConnection();
  if (m_snSend != NULL) m_snSend->setEnabled(false);
  lblStatus->setText(_("File transfer complete."));
  btnCancel->setText(_("Done"));
}


//=====Other Stuff==============================================================

//-----hide---------------------------------------------------------------------
void CFileDlg::hide()
{
   QWidget::hide();
   fileCancel();
   delete this;
}


//-----resizeEvent----------------------------------------------------
void CFileDlg::resizeEvent (QResizeEvent *)
{
   // resize / reposition all the widgets
   //barTransfer->setGeometry(10, height() - 90, (width() >> 1) - 15, 20);
   //barBatchTransfer->setGeometry((width() >> 1) + 5, height() - 90, (width() >> 1) - 15, 20);
   lblStatus->setGeometry(0, height() - 20, width(), 20);
   btnCancel->setGeometry((width() >> 1) - 100, height() - 60, 200, 30);
}

#include "moc/moc_filedlg.h"
