/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <qfiledialog.h>
#include <qdir.h>
#include <qhbox.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qprogressbar.h>
#include <qsocketnotifier.h>

#include "filedlg.h"
#include "licq_packets.h"
#include "licq_filetransfer.h"
#include "licq_log.h"
#include "licq_translate.h"
#include "ewidgets.h"
#include "licq_icqd.h"



//-----Constructor------------------------------------------------------------
CFileDlg::CFileDlg(unsigned long _nUin, CICQDaemon *daemon,
                   QWidget *parent, char *name)
  : QDialog(parent, name)
{
  // If we are the server, then we are receiving a file
  m_nUin = _nUin;
  licqDaemon = daemon;

  setCaption(tr("Licq - File Transfer (%1)").arg(m_nUin));

  unsigned short CR = 0;
  QGridLayout* lay = new QGridLayout(this, 8, 3, 8, 8);
  lay->setColStretch(1, 2);

  lblTransferFileName = new QLabel(tr("Current:"), this);
  lay->addWidget(lblTransferFileName, CR, 0);
  nfoTransferFileName = new CInfoField(this, true);
  nfoTransferFileName->setMinimumWidth(nfoTransferFileName->sizeHint().width()*2);
  lay->addWidget(nfoTransferFileName, CR, 1);
  nfoTotalFiles = new CInfoField(this, true);
  nfoTotalFiles->setMinimumWidth((nfoTotalFiles->sizeHint().width()*3)/2);
  lay->addWidget(nfoTotalFiles, CR, 2);

  lblLocalFileName = new QLabel(tr("File name:"), this);
  lay->addWidget(lblLocalFileName, ++CR, 0);
  nfoLocalFileName = new CInfoField(this, true);
  lay->addMultiCellWidget(nfoLocalFileName, CR, CR, 1, 2);

  lay->addRowSpacing(++CR, 10);

  // Information stuff about the current file
  lblTrans = new QLabel(tr("File:"), this);
  lay->addWidget(lblTrans, ++CR, 0);
  barTransfer = new QProgressBar(this);
  lay->addWidget(barTransfer, CR, 1);
  nfoFileSize = new CInfoField(this, true);
  lay->addWidget(nfoFileSize, CR, 2);

  // Information about the batch file transfer
  lblBatch = new QLabel(tr("Batch:"), this);
  lay->addWidget(lblBatch, ++CR, 0);
  barBatchTransfer = new QProgressBar(this);
  lay->addWidget(barBatchTransfer, CR, 1);
  nfoBatchSize = new CInfoField(this, true);
  lay->addWidget(nfoBatchSize, CR, 2);

  lblTime = new QLabel(tr("Time:"), this);
  lay->addWidget(lblTime, ++CR, 0);

  QHBox* hbox = new QHBox(this);
  hbox->setSpacing(8);
  lay->addMultiCellWidget(hbox, CR, CR, 0, 1);
  nfoTime = new CInfoField(hbox, true);
  nfoBPS = new CInfoField(hbox, true);
  lblETA = new QLabel(tr("ETA:"), hbox);
  nfoETA = new CInfoField(this, true);
  lay->addWidget(nfoETA, CR++, 2);
  lay->addRowSpacing(++CR, 10);

  lblStatus = new QLabel(this);
  ++CR;
  lay->addMultiCellWidget(lblStatus, CR, CR, 0, 2);
  lblStatus->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);

  lay->setRowStretch(++CR, 3);

  btnCancel = new QPushButton(tr("&Cancel Transfer"), this);
  btnCancel->setMinimumWidth(75);
  lay->addMultiCellWidget(btnCancel, CR, CR, 1, 2);
  connect(btnCancel, SIGNAL(clicked()), this, SLOT(hide()));

  ftman = new CFileTransferManager(licqDaemon, m_nUin);
  ftman->SetUpdatesEnabled(2);
  sn = new QSocketNotifier(ftman->Pipe(), QSocketNotifier::Read);
  connect(sn, SIGNAL(activated(int)), SLOT(slot_ft()));
}


//-----Destructor---------------------------------------------------------------
CFileDlg::~CFileDlg()
{
  delete sn;
  delete ftman;
}


unsigned short CFileDlg::LocalPort()
{
  return ftman->LocalPort();
}


//-----fileCancel---------------------------------------------------------------
void CFileDlg::slot_cancel()
{
  // close the local file and other stuff
  if (sn != NULL) sn->setEnabled(false);
  lblStatus->setText(tr("File transfer cancelled"));
  btnCancel->setText(tr("Close"));
  ftman->CloseFileTransfer();
}


//-----fileUpdate---------------------------------------------------------------
void CFileDlg::slot_update()
{
  //update time, BPS and eta
  static char sz[16];

  // Current File

  // Transfered
  nfoFileSize->setText(QString(tr("%1/%2"))
     .arg(encodeFSize(ftman->FilePos())).arg(encodeFSize(ftman->FileSize())));

  // Time
  time_t nTime = time(NULL) - ftman->StartTime();
  sprintf(sz, "%02ld:%02ld:%02ld", nTime / 3600, (nTime % 3600) / 60, (nTime % 60));
  nfoTime->setText(sz);
  if (nTime == 0 || ftman->BytesTransfered() == 0)
  {
    nfoBPS->setText("---");
    nfoETA->setText("---");
    return;
  }

  // BPS
  nfoBPS->setText(QString("%1/s").arg(encodeFSize(ftman->BytesTransfered() / nTime)));

  // ETA
  int nBytesLeft = ftman->FileSize() - ftman->FilePos();
  time_t nETA = (time_t)(nBytesLeft / (ftman->BytesTransfered() / nTime));
  sprintf(sz, "%02ld:%02ld:%02ld", nETA / 3600, (nETA % 3600) / 60, (nETA % 60));
  nfoETA->setText(sz);

  // Batch

  // Transfered
  nfoBatchSize->setText(QString(tr("%1/%2"))
     .arg(encodeFSize(ftman->BatchPos())).arg(encodeFSize(ftman->BatchSize())));


  barTransfer->setProgress(ftman->FilePos());
  barBatchTransfer->setProgress(ftman->BatchPos());
}




//-----ReceiveFiles----------------------------------------------------------
bool CFileDlg::ReceiveFiles()
{
  // Get the local directory to save to
  QString d;
  d = QFileDialog::getExistingDirectory(QString(QDir::homeDirPath()), this);
  if (d.isNull()) return false;

  if (!ftman->ReceiveFiles(d.latin1())) return false;

  lblStatus->setText(tr("Waiting for connection..."));
  show();
  return true;
}


//-----CFileDlg::slot_ft-----------------------------------------------------
void CFileDlg::slot_ft()
{
  // Read out any pending events
  char buf[32];
  read(ftman->Pipe(), buf, 32);

  CFileTransferEvent *e = NULL;
  while ( (e = ftman->PopFileTransferEvent()) != NULL)
  {
    switch(e->Command())
    {
      case FT_STARTxBATCH:
      {
        setCaption(tr("Licq - File Transfer (%1)").arg(ftman->RemoteName()));
        nfoTotalFiles->setText(tr("%1 / %2").arg(1).arg(ftman->BatchFiles()));
        nfoBatchSize->setText(encodeFSize(ftman->BatchSize()));
        barBatchTransfer->setTotalSteps(ftman->BatchSize());
        barBatchTransfer->setProgress(0);
        break;
      }

      case FT_STARTxFILE:
      {
        nfoTotalFiles->setText(tr("%1 / %2").arg(ftman->CurrentFile()).arg(ftman->BatchFiles()));
        nfoTransferFileName->setText(ftman->FileName());
        nfoLocalFileName->setText(ftman->PathName());
        nfoFileSize->setText(encodeFSize(ftman->FileSize()));
        barTransfer->setTotalSteps(ftman->FileSize());
        if (ftman->Direction() == D_RECEIVER)
          lblStatus->setText(tr("Receiving file..."));
        else
          lblStatus->setText(tr("Sending file..."));
        break;
      }

      case FT_UPDATE:
      {
        slot_update();
        break;
      }

      case FT_DONExFILE:
      {
        lblStatus->setText(tr("Done %1").arg(e->Data()));
        slot_update();
        if (ftman->Direction() == D_RECEIVER)
          InformUser(this, tr("Received\n%1\nfrom %2 successfully").arg(e->Data()).arg(ftman->RemoteName()));
        else
          InformUser(this, tr("Sent\n%1\nto %2 successfully").arg(e->Data()).arg(ftman->RemoteName()));
        break;
      }

      case FT_DONExBATCH:
      {
        lblStatus->setText(tr("File transfer complete"));
        btnCancel->setText(tr("Ok"));
        ftman->CloseFileTransfer();
        break;
      }

      case FT_ERRORxCLOSED:
      {
        btnCancel->setText(tr("Close"));
        lblStatus->setText(tr("Remote side disconnected"));
        ftman->CloseFileTransfer();
        WarnUser(this, tr("Remote side disconnected"));
        break;
      }

      case FT_ERRORxFILE:
      {
        btnCancel->setText(tr("Close"));
        lblStatus->setText(tr("File I/O error: %1").arg(ftman->PathName()));
        ftman->CloseFileTransfer();
        WarnUser(this, tr("File I/O Error:\n%1\nSee Network Window for Details")
           .arg(ftman->PathName()));
        break;
      }

      case FT_ERRORxHANDSHAKE:
      {
        btnCancel->setText(tr("Close"));
        lblStatus->setText(tr("Handshaking error"));
        ftman->CloseFileTransfer();
        WarnUser(this, tr("Handshake Error\nSee Network Window for Details"));
        break;
      }
    }

    delete e;
  }
}



//-----CFileDlg::SendFiles---------------------------------------------------
bool CFileDlg::SendFiles(const char *szFile, unsigned short nPort)
{
  ConstFileList fl;
  fl.push_back(szFile);
  if (!ftman->SendFiles(fl, nPort)) return false;

  lblStatus->setText(tr("Connecting to remote..."));
  show();
  return true;
}




//-----hide--------------------------------------------------------------------
void CFileDlg::hide()
{
   QWidget::hide();
   delete this;
}

// -----------------------------------------------------------------------------

QString CFileDlg::encodeFSize(unsigned long size)
{
  QString unit;
  if(size >= 1024*1024) {
    size /= (1024*1024)/10;
    unit = tr("MB");
  }
  else if(size >= 1024) {
    size /= 1024/10;
    unit = tr("KB");
  }
  else if(size != 1)
    unit = tr("Bytes");
  else
    unit = tr("Byte");

  return QString("%1.%2 %3").arg(size/10).arg(size%10).arg(unit);
}

#include "filedlg.moc"


#if 0
//-----GetLocalFileName---------------------------------------------------------
// Sets the local filename and opens the file
// returns false if the user hits cancel
bool CFileDlg::GetLocalFileName()
{
  QString f;
  bool bValid = false;
  // Get the local filename and open it, loop until valid or cancel
  while(!bValid)
  {
#ifdef USE_KDE
    KURL u = KFileDialog::getSaveURL(
          QString(QDir::homeDirPath() + "/" + m_sFileInfo.szName),
          QString::null, this);
    f = u.path();
#else
    f = QFileDialog::getSaveFileName(
          QString(QDir::homeDirPath() + "/" + m_sFileInfo.szName),
          QString::null, this);
#endif
    if (f.isNull()) return (false);
    struct stat buf;
    int nFlags = O_WRONLY;
    m_nFilePos = 0;
    if (stat(QFile::encodeName(f), &buf) == 0)  // file already exists
    {
      if ((unsigned long)buf.st_size >= m_sFileInfo.nSize)
      {
        if(QueryUser(this, tr("File already exists and is at least as big as the incoming file."), tr("Overwrite"), tr("Cancel")))
          nFlags |= O_TRUNC;
        else
          return (false);
      }
      else
      {
        if (QueryUser(this, tr("File already exists and appears incomplete."), tr("Overwrite"), tr("Resume")))
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

    m_nFileDesc = open (QFile::encodeName(f), nFlags, 00664);
    if (m_nFileDesc < 0)
    {
      if (!QueryUser(this, tr("Open error - unable to open file for writing."), tr("Retry"), tr("Cancel")))
        return (false);
    }
    else
      bValid = true;
  }

  nfoLocalFileName->setText(f);
  m_nBytesTransfered = 0;
  barTransfer->setTotalSteps(m_sFileInfo.nSize);
  barTransfer->setProgress(0);
  return(true);
}
#endif

