#ifndef MAINWIN_H
#define MAINWIN_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_KDE
#include <kmenubar.h>
#else
#include <qmenubar.h>
#endif

#if QT_VERSION < 300
  #include <qlist.h>
#else
  #include <qptrlist.h>
#endif
#include <qwidget.h>
#include <qtimer.h>
#include <qbitmap.h>
#include <qdialog.h>

#include "userbox.h"
#include "licq_user.h"
#include "gui-defines.h"

class QStyle;

class QTextView;
class CSignalManager;
class CQtLogWindow;
class CSkin;
class CICQDaemon;
class ICQEvent;
class OptionsDlg;
class AwayMsgDlg;
class RegisterUserDlg;
class CUserView;
class CELabel;
class CEButton;
class CEComboBox;
class QListViewItem;
class UserViewEvent;
class UserEventCommon;
class UserSendCommon;
#ifdef USE_DOCK
class IconManager;
#endif

class CICQSignal;
class UserInfoDlg;

#if QT_VERSION < 300
  typedef QList<UserViewEvent> UserViewEventList;
  typedef QList<UserInfoDlg> UserInfoList;
  typedef QList<UserSendCommon> UserSendEventList;
#else
  typedef QPtrList<UserViewEvent> UserViewEventList;
  typedef QPtrList<UserInfoDlg> UserInfoList;
  typedef QPtrList<UserSendCommon> UserSendEventList;
#endif

//=====CMainWindow==============================================================
class CMainWindow : public QWidget
{
  Q_OBJECT
  friend class CLicqGui;

public:
  CMainWindow(CICQDaemon *theServer, CSignalManager *theSigMan,
              CQtLogWindow *theLogWindow, bool bStartHidden,
              const char *skinName, const char *iconsName,
              QWidget *parent = 0);
  virtual ~CMainWindow();
  UserEventCommon *callFunction(int fcn, unsigned long nUin);
  bool RemoveUserFromList(unsigned long, QWidget *);
  bool RemoveUserFromGroup(GroupType gtype, unsigned long group, unsigned long, QWidget *);
  void ApplySkin(const char *, bool = false);
  void ApplyIcons(const char *, bool = false);
  CUserView *UserView()  { return userView; }
  QPopupMenu *UserMenu() { return mnuUser; }
  void SetUserMenuUin(unsigned long n) { m_nUserMenuUin = n; }

  static QPixmap &iconForStatus(unsigned long FullStatus);
  static QPixmap &iconForEvent(unsigned short SubCommand);

  // global configuration data
  // Toggles
  bool m_bInMiniMode,
       m_bGridLines,
       m_bFontStyles,
       m_bShowGroupIfNoMsg,
       m_bShowHeader,
       m_bAutoClose,
       m_bAutoPopup,
       m_bShowOffline,
       m_bAlwaysShowONU,
       m_bSortByStatus,
       m_bShowDividers,
       m_bAutoRaise,
       m_bHidden,
       m_bBoldOnMsg,
       m_bManualNewUser,
       m_bThreadView,
       m_bScrollBar,
       m_bShowExtendedIcons,
       m_bSystemBackground,
       m_bSendFromClipboard,
       m_bMsgChatView,
       m_bAutoPosReplyWin,
       m_bAutoSendThroughServer,
       m_bEnableMainwinMouseMovement,
       m_bPopEmail,
       m_bPopPhone, 
       m_bPopFax, 
       m_bPopCellular, 
       m_bPopIP, 
       m_bPopLastOnline,
       m_bPopOnlineSince,
       m_bShowAllEncodings;

  QString m_MsgAutopopupKey;
  QString m_DefaultEncoding;

  ColumnInfos colInfo;
  FlashType m_nFlash;
  CSkin *skin;

  unsigned long m_nCurrentGroup, m_nGroupStates;
  GroupType m_nGroupType;
  QString usprintfHelp;

public slots:
  void callInfoTab(int, unsigned long, bool toggle=false);

public:
  // Command Tools
  CICQDaemon *licqDaemon;
  CSignalManager *licqSigMan;
  CQtLogWindow *licqLogWindow;
#ifdef USE_DOCK
  IconManager *licqIcon;
#endif
  UserViewEventList licqUserView;
  UserInfoList licqUserInfo;
  UserSendEventList licqUserSend;

  // Dialog boxes
  AwayMsgDlg *awayMsgDlg;
  OptionsDlg *optionsDlg;
  RegisterUserDlg *registerUserDlg;

  // Widgets
  CUserView *userView;
#ifdef USE_KDE
  KMenuBar *menu;
#else
  QMenuBar *menu;
#endif
  QPopupMenu *mnuSystem,
             *mnuUser,
             *mnuUserGroups,
             *mnuGroup,
             *mnuOwnerAdm,
             *mnuUserAdm,
             *mnuStatus,
             *mnuDebug,
             *mnuUtilities,
             *mnuMiscModes,
             *mnuSend;
  CELabel *lblStatus, *lblMsg;
  CEButton *btnSystem;
  CEComboBox *cmbUserGroups;

  QPixmap *pmBorder, *pmMask;
  QBitmap bmMask;

  // GUI Data
  QString m_szCaption;
  QFont defaultFont;
  QStyle *style;
  unsigned short m_nAutoLogon;
  char *m_szIconSet;
  QPixmap pmOnline, pmOffline, pmAway, pmDnd, pmOccupied, pmNa,
          pmPrivate, pmFFC, pmMessage, pmUrl, pmChat, pmFile, pmContact, pmSms,
          pmAuthorize, pmSMS, pmSecureOn, pmSecureOff, pmHistory, pmInfo, pmEncoding;
  unsigned long m_nUserMenuUin;
  unsigned int positionChanges;

  // AutoAway
  QTimer autoAwayTimer;
  unsigned short autoAwayTime,
                 autoNATime,
                 autoOfflineTime,
                 manualAway,
                 m_nRealHeight,
                 autoAwayMess,
                 autoNAMess;

  DockMode m_nDockMode;
  int mouseX, mouseY;

  // Functions
  void CreateUserView();
  void CreateUserFloaty(unsigned long nUin, unsigned short x = 0,
     unsigned short y = 0, unsigned short w = 0);
  void initMenu();
  bool show_user(ICQUser *);

  virtual void resizeEvent (QResizeEvent *);
  virtual void moveEvent(QMoveEvent*);
  virtual void mouseMoveEvent (QMouseEvent *);
  virtual void mousePressEvent (QMouseEvent *);
  virtual void closeEvent (QCloseEvent *);
  virtual void keyPressEvent(QKeyEvent *e);

  friend class WharfIcon;
  friend class IconManager;
  friend class IconManager_Default;
  friend class OptionsDlg;
  friend class CUserViewItem;
  friend class UserSendCommon;

public slots:
  void updateUserWin();
  void slot_shutdown();
  void saveOptions();
  void slot_updatedList(CICQSignal *);
  void slot_updatedUser(CICQSignal *);

protected slots:
  void slot_hints();
  void UserGroupToggled(int);
  void FillUserGroup();
  void saveAllUsers();
  void updateEvents();
  void updateStatus();
  void updateGroups();
  void changeStatus(int index);
  void changeStatusManual(int index);
  void setCurrentGroupMenu(int id);
  void setCurrentGroup(int);
  void callDefaultFunction(QListViewItem *);
  void callOwnerFunction(int);
  void callMsgFunction();
  void callFileFunction (const char *);
  void callUrlFunction (const char *);
  void callUserFunction(int);
  void slot_userfinished(unsigned long);
  void slot_sendfinished(unsigned long);
  void slot_usermenu();
  void slot_logon();
  void slot_ui_viewevent(unsigned long);
  void slot_ui_message(unsigned long);
  void slot_register();
  void slot_doneregister();
  void slot_doneOptions();
  void slot_doneOwnerFcn(ICQEvent *);
  void slot_doneAwayMsgDlg();
  void slot_stats();
  void showAddUserDlg();
  void showEditGrpDlg();
  void showSkinBrowser();
  void showPluginDlg();
  void slot_AwayMsgDlg();
  void showAwayMsgDlg(unsigned short status);
  void showOptionsDlg(int);
  void popupOptionsDlg() { emit showOptionsDlg(0); };
  void showAuthUserDlg();
  void showReqAuthDlg(unsigned long nUin = 0);
  void showSearchUserDlg();
  void popupSystemMenu();
  void changeDebug(int);
  void slot_utility(int);
  void slot_miscmodes(int);
  void slot_randomchatsearch();
  void ToggleMiniMode();
  void ToggleShowOffline();
  void ToggleThreadView();
  void nextServer();
  void autoAway();
  void aboutBox();
  void slot_updateContactList();
  void slot_updateAllUsers();
  void slot_updateAllUsersInGroup();
  void slot_popupall();
  void slot_aboutToQuit();
  void UserInfoDlg_finished(unsigned long);

signals:
  void changeDockStatus(unsigned short);
  void signal_sentevent(ICQEvent *e);

};

// -----------------------------------------------------------------------------

class HintsDlg : public QDialog
{
public:
  HintsDlg(QString &);

private:
  QTextView *txtView;
  QPushButton *btnClose;
};

// -----------------------------------------------------------------------------

extern CMainWindow* gMainWindow;

// HintsDlg class

#endif
