#ifndef AWAYMSG_H
#define AWAYMSG_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <qdialog.h>
#include <qmultilineedit.h>
#include <qcheckbox.h>
#include <qpushbutton.h>

#include "user.h"

class ShowAwayMsgDlg : public QDialog
{
  Q_OBJECT
public:
  ShowAwayMsgDlg(unsigned long _nUin, QWidget *parent = 0, const char *name = 0);

protected:
  unsigned long m_nUin;
  QMultiLineEdit *qleAwayMsg;
  QCheckBox *chkShowAgain;
  QPushButton *btnOk;
  
protected slots:
  virtual void accept();

};


#endif
