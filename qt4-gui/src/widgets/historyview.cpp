// -*- c-basic-offset: 2 -*-
/*
 * This file is part of Licq, an instant messaging client for UNIX.
 * Copyright (C) 2007 Licq developers
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

#include "historyview.h"

#include <QDateTime>
#include <QRegExp>
#include <QTextCodec>

#include <licq_events.h>
#include <licq_user.h>

#include "config/chat.h"

#include "helpers/eventdesc.h"
#include "helpers/usercodec.h"

using namespace LicqQtGui;
/* TRANSLATOR LicqQtGui::HistoryView */

QStringList HistoryView::getStyleNames(bool includeHistoryStyles)
{
  static const char* const styleNames[] = {
    QT_TR_NOOP("Default"),
    QT_TR_NOOP("Compact"),
    QT_TR_NOOP("Tiny"),
    QT_TR_NOOP("Table"),
    QT_TR_NOOP("Long"),
    QT_TR_NOOP("Wide")
  };

  int listLength = 6;

  // Style 5 (Wide) is currently only supported in buffered mode which is not used for chat
  if (!includeHistoryStyles)
    listLength--;

  QStringList styleList;
  for (int i = 0; i < listLength; ++i)
    styleList.append(tr(styleNames[i]));

  return styleList;
}

HistoryView::HistoryView(bool historyMode, QString id, unsigned long ppid, QWidget* parent)
  : MLView(parent),
    myId(id),
    myPpid(ppid)
{
  Config::Chat* chatConfig = Config::Chat::instance();
  if (historyMode)
  {
    setHistoryConfig(chatConfig->histMsgStyle(), chatConfig->histDateFormat(),
        chatConfig->histVertSpacing(), chatConfig->reverseHistory());
  }
  else
  {
    setChatConfig(chatConfig->chatMsgStyle(), chatConfig->chatDateFormat(),
        chatConfig->chatVertSpacing(), chatConfig->chatAppendLineBreak(),
        chatConfig->showNotices());
  }

  setColors();
  connect(chatConfig, SIGNAL(chatColorsChanged()), SLOT(setColors()));

  clear();
}

HistoryView::~HistoryView()
{
}

QSize HistoryView::sizeHint() const
{
  // Set a size hint wide enough for history to be readable
  return QSize(400, 150);
}

void HistoryView::setHistoryConfig(unsigned short msgStyle, QString dateFormat, bool extraSpacing, bool reverse)
{
  myUseBuffer = true;
  myMsgStyle = msgStyle;
  myDateFormat = dateFormat;
  myExtraSpacing = extraSpacing;
  myReverse = reverse;
  myAppendLineBreak = false;
  myShowNotices = false;
}

void HistoryView::setChatConfig(unsigned short msgStyle, QString dateFormat, bool extraSpacing, bool appendLineBreak, bool showNotices)
{
  myUseBuffer = false;
  myMsgStyle = msgStyle;
  myDateFormat = dateFormat;
  myExtraSpacing = extraSpacing;
  myReverse = false;
  myAppendLineBreak = appendLineBreak;
  myShowNotices = showNotices;
}

void HistoryView::setColors(QString back, QString rcv, QString snt,
    QString rcvHist, QString sntHist, QString notice)
{
  myColorRcv = rcv;
  myColorSnt = snt;
  if (!rcvHist.isEmpty())
    myColorRcvHistory = rcvHist;
  if (!sntHist.isEmpty())
    myColorSntHistory = sntHist;
  if (!notice.isEmpty())
    myColorNotice = notice;
  if (!back.isEmpty())
    setBackground(QColor(back));
}

void HistoryView::setColors()
{
  Config::Chat* chatConfig = Config::Chat::instance();

  setColors(
      chatConfig->chatBackColor(),
      chatConfig->recvColor(),
      chatConfig->sentColor(),
      chatConfig->recvHistoryColor(),
      chatConfig->sentHistoryColor(),
      chatConfig->noticeColor()
  );
}

void HistoryView::setReverse(bool reverse)
{
  myReverse = reverse;
}

void HistoryView::setOwner(QString id, unsigned long ppid)
{
  myId = id;
  if (ppid != 0)
    myPpid = ppid;
}

void HistoryView::clear()
{
  MLView::clear();

  myBuffer = "";

  switch (myMsgStyle)
  {
    case 5:
      // table doesn't work when appending so must buffer when using this style
      myUseBuffer = true;
      break;
  }
}

void HistoryView::updateContent()
{
  if (!myUseBuffer)
    return;

  switch (myMsgStyle)
  {
    case 5:
      // When myReverse is set (so that we prepend() to the buffer),
      // we cannot put the <table> tag in HistoryView::clear().
      // That's why we are prepend()'ing it here, in updateContent().
      // Then, if we combine incremenetal updateContent()'s with
      // myMsgStyle == 5 (we don't so far), we'll obtain several
      // <table>'s in the buffer -- this works, but stinks heavily.
      myBuffer.prepend("<table border=\"0\">");
      break;
  }
  // actually, we don't need it at all
  // myBuffer.prepend("<html><body>");

  setText(myBuffer);
}

void HistoryView::internalAddMsg(QString s)
{
  if (myExtraSpacing)
  {
    if (myMsgStyle != 5)
    {
      if (myUseBuffer)
      {
        s.prepend("<p>");
        s.append("</p>");
      }
      else
      {
        s.append("<br>");
      }
    }
    else
    {
      s.append("<tr><td colspan=\"3\"></td></tr>");
    }
  }

  if (myUseBuffer)
  {
    if (!myExtraSpacing && myMsgStyle != 5)
      s.append("<br>");

    if (myAppendLineBreak)
      s.append("<hr>");

    if (myReverse)
      myBuffer.prepend(s);
    else
      myBuffer.append(s);
  }
  else
  {
    if (myAppendLineBreak)
      s.append("<hr>");

    append(s);
  }
}

void HistoryView::addMsg(const ICQEvent* event)
{
  if (event->Id() == myId && event->PPID() == myPpid && event->UserEvent() != NULL)
    addMsg(event->UserEvent());
}

void HistoryView::addMsg(direction dir, bool fromHistory, QString eventDescription, QDateTime date,
  bool isDirect, bool isMultiRec, bool isUrgent, bool isEncrypted,
  QString contactName, QString messageText, QString anchor)
{
  QString s;
  QString color;

  if (fromHistory)
  {
    if (dir == D_RECEIVER)
      color = myColorRcvHistory;
    else
      color = myColorSntHistory;
  }
  else
  {
    if (dir == D_RECEIVER)
      color = myColorRcv;
    else
      color = myColorSnt;
  }

  // Remove trailing line breaks.
  for (int i = messageText.length(); i > 0; --i)
  {
    if (messageText.at(i - 1) != '\n' && messageText.at(i - 1) != '\r')
    {
      messageText.truncate(i);
      break;
    }
  }

  // Extract everything inside <body>...</body>
  // Leaving <html> and <body> messes with our message display
  QRegExp body("<body[^>]*>(.*)</body>");
  if (body.indexIn(messageText) != -1)
    messageText = body.cap(1);

  // Remove all font tags
  messageText.replace(QRegExp("</?font[^>]*>"), "");

  QString dateString = date.toString(myDateFormat);

  if (!anchor.isEmpty())
    anchor = "<a name=\"" + anchor + "\"/>";

  QString flags = QString("%1%2%3%4")
      .arg(isDirect ? 'D' : '-')
      .arg(isMultiRec ? 'M' : '-')
      .arg(isUrgent ? 'U' : '-')
      .arg(isEncrypted ? 'E' : '-');

  switch (myMsgStyle)
  {
    case 0:
      s = QString("%1<font color=\"%2\"><b>%3%4 [%5] %6:</b></font><br>")
          .arg(anchor)
          .arg(color)
          .arg(eventDescription)
          .arg(dateString)
          .arg(flags)
          .arg(contactName);
      s.append(QString("<font color=\"%1\">%2</font>")
          .arg(color)
          .arg(messageText));
      break;
    case 1:
      s = QString("%1<font color=\"%2\"><b>(%3%4) [%5] %6: </b></font>")
          .arg(anchor)
          .arg(color)
          .arg(eventDescription)
          .arg(dateString)
          .arg(flags)
          .arg(contactName);
      s.append(QString("<font color=\"%1\">%2</font>")
          .arg(color)
          .arg(messageText));
      break;
    case 2:
      s = QString("%1<font color=\"%2\"><b>%3%4 - %5: </b></font>")
          .arg(anchor)
          .arg(color)
          .arg(eventDescription)
          .arg(dateString)
          .arg(contactName);
      s.append(QString("<font color=\"%1\">%2</font>")
          .arg(color)
          .arg(messageText));
      break;
    case 3:
      s = QString("%1<table border=\"1\"><tr><td><b><font color=\"%2\">%3%4</font><b><td><b><font color=\"%5\">%6</font></b></font></td>")
          .arg(anchor)
          .arg(color)
          .arg(eventDescription)
          .arg(dateString)
          .arg(color)
          .arg(contactName);
      s.append(QString("<td><font color=\"%1\">%2</font></td></tr></table>")
          .arg(color)
          .arg(messageText));
      break;
    case 4:
      s = QString("%1<font color=\"%2\"><b>%3 %4 %5<br>%6 [%7]</b></font><br><br>")
          .arg(anchor)
          .arg(color)
          .arg(eventDescription)
          .arg(dir == D_RECEIVER ? tr("from") : tr("to"))
          .arg(contactName)
          .arg(dateString)
          .arg(flags);

      // We break the paragraph here, since the history text
      // could be in a different BiDi directionality than the
      // header and timestamp text.
      s.append(QString("<font color=\"%1\">%2</font><br><br>")
          .arg(color)
          .arg(messageText));
      break;
    case 5:
      // Mode 5 is a table so it cannot be displayed in paragraphs
      s = QString("<tr><td>%1<nobr><b><font color=\"%2\">%3</font><b> </nobr></td>")
          .arg(anchor)
          .arg(color)
          .arg(dateString);
      s.append(QString("<td><b><font color=\"%1\">%2</font></b></font>&nbsp;</td>")
          .arg(color)
          .arg(contactName));
      s.append(QString("<td><font color=\"%1\">%2</font></td></tr>")
          .arg(color)
          .arg(messageText));
      break;
  }

  internalAddMsg(s);
}

void HistoryView::addMsg(const CUserEvent* event, QString id, unsigned long ppid)
{
  QDateTime date;
  date.setTime_t(event->Time());
  QString sd = date.time().toString(myDateFormat);
  bool bUseHTML = false;

  QString contactName;
  QTextCodec* codec = NULL;

  if (id.isNull())
  {
    id = myId;
    ppid = myPpid;
  }

  const ICQUser* u = gUserManager.FetchUser(id.toLatin1(), ppid, LOCK_R);
  if (u != NULL)
  {
    codec = UserCodec::codecForICQUser(u);
    if (event->Direction() == D_RECEIVER)
    {
      contactName = QString::fromUtf8(u->GetAlias());
      if (myPpid == LICQ_PPID)
        for (int x = 0; x < myId.length(); ++x)
          if (!myId.at(x).isDigit())
          {
            bUseHTML = true;
            break;
          }
    }
    gUserManager.DropUser(u);
  }

  if (event->Direction() != D_RECEIVER)
  {
    ICQOwner* o = gUserManager.FetchOwner(myPpid, LOCK_R);
    if (o != NULL)
    {
      contactName = QString::fromUtf8(o->GetAlias());
      gUserManager.DropOwner(o);
    }
  }

  // Fallback, in case we couldn't fetch User.
  if (codec == NULL)
    codec = QTextCodec::codecForName("UTF-8");

  QString messageText;
  if (event->SubCommand() == ICQ_CMDxSUB_SMS)
    messageText = QString::fromUtf8(event->Text());
  else
    messageText = codec->toUnicode(event->Text());

  addMsg(event->Direction(), false,
         (event->SubCommand() == ICQ_CMDxSUB_MSG ? QString("") : (EventDescription(event) + " ")),
         date,
         event->IsDirect(),
         event->IsMultiRec(),
         event->IsUrgent(),
         event->IsEncrypted(),
         contactName,
         MLView::toRichText(messageText, true, bUseHTML));
  GotoEnd();

  if (event->Direction() == D_RECEIVER &&
      (event->SubCommand() == ICQ_CMDxSUB_MSG ||
       event->SubCommand() == ICQ_CMDxSUB_URL))
    emit messageAdded();
}

void HistoryView::addNotice(QDateTime dt, QString messageText)
{
  if (!myShowNotices)
    return;

  QString color = myColorNotice;
  QString s = "";
  const QString dateTime = dt.toString(myDateFormat);

  // Remove trailing line breaks.
  for (int i = messageText.length(); i >= 0; --i)
  {
    if (messageText.at(i - 1) != '\n' && messageText.at(i - 1) != '\r')
    {
      messageText.truncate(i);
      break;
    }
  }

  switch (myMsgStyle)
  {
    case 1:
      s = QString("<font color=\"%1\"><b>[%2] %3</b></font>")
          .arg(color)
          .arg(dateTime)
          .arg(messageText);
      break;
    case 2:
      s = QString("<font color=\"%1\"><b>[%2] %3</b></font>")
          .arg(color)
          .arg(dateTime)
          .arg(messageText);
      break;
    case 3:
      s = QString("<table border=\"1\"><tr><td><b><font color=\"%1\">%2</font><b><td><b><font color=\"%3\">%4</font></b></font></td></tr></table>")
          .arg(color)
          .arg(dateTime)
          .arg(color)
          .arg(messageText);
      break;

    case 5:
      s = QString("<tr><td><b><font color=\"%1\">%2</font><b></td><td colspan=\"2\"><b><font color=\"%3\">%4</font></b></font></td></tr>")
          .arg(color)
          .arg(dateTime)
          .arg(color)
          .arg(messageText);
      break;

    case 0:
    default:
      s = QString("<font color=\"%1\"><b>[%2] %3</b></font><br>")
          .arg(color)
          .arg(dateTime)
          .arg(messageText);
      break;
  }

  internalAddMsg(s);
  GotoEnd();
}
