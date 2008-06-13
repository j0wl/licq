// -*- c-basic-offset: 2 -*-
/*
 * This file is part of Licq, an instant messaging client for UNIX.
 * Copyright (C) 1999-2006 Licq developers
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

#include "searchuserdlg.h"

#include "config.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextCodec>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <licq_user.h>
#include <licq_icqd.h>
#include <licq_languagecodes.h>
#include <licq_countrycodes.h>

#include "core/gui-defines.h"
#include "core/licqgui.h"
#include "core/messagebox.h"
#include "core/signalmanager.h"

#include "dialogs/adduserdlg.h"

#include "helpers/support.h"

using namespace LicqQtGui;
/* TRANSLATOR LicqQtGui::SearchUserDlg */

SearchUserDlg::SearchUserDlg()
  : ppid(LICQ_PPID),
    searchTag(0)
{
  Support::setWidgetProps(this, "SearchUserDialog");
  setAttribute(Qt::WA_DeleteOnClose, true);
  setWindowTitle(tr("Licq - User Search"));

  connect(LicqGui::instance()->signalManager(),
      SIGNAL(searchResult(ICQEvent*)), SLOT(searchResult(ICQEvent*)));

  QVBoxLayout* lay = new QVBoxLayout(this);

  // pre-search widgets
  grpParms = new QGroupBox(tr("Search Criteria"));
  lay->addWidget(grpParms);

  QGridLayout* grp_lay = new QGridLayout(grpParms);
  grp_lay->setColumnMinimumWidth(3, 10); // column interspacing

  QList<QComboBox*> combos;

  QStringList ages = QStringList()
    << tr("Unspecified")
    << "18 - 22"
    << "23 - 29"
    << "30 - 39"
    << "40 - 49"
    << "50 - 59"
    << "60+";

  QStringList genders = QStringList()
    << tr("Unspecified")
    << tr("Female")
    << tr("Male");

  QStringList languages;
  for (int i = 0; i < NUM_LANGUAGES; i++)
    languages << GetLanguageByIndex(i)->szName;

  QStringList countries;
  for (int i = 0; i < NUM_COUNTRIES; i++)
    countries << GetCountryByIndex(i)->szName;

  int row = 0;
  int column = 0;

  QLabel* label;

#define ADDFULLLINE(name, var) \
  label = new QLabel(name); \
  var = new QLineEdit(); \
  label->setBuddy(var); \
  grp_lay->addWidget(label, row, 0); \
  grp_lay->addWidget(var, row++, 2, 1, 5);

#define ADDLINE(name, var) \
  label = new QLabel(name); \
  var = new QLineEdit(); \
  label->setBuddy(var); \
  grp_lay->addWidget(label, row, column); \
  grp_lay->addWidget(var, row++, column + 2);

#define ADDCMB(name, var, list) \
  label = new QLabel(name); \
  var = new QComboBox(); \
  label->setBuddy(var); \
  grp_lay->addWidget(label, row, column); \
  grp_lay->addWidget(var, row++, column + 2); \
  (var)->addItems((list)); combos << (var);

  ADDFULLLINE(tr("UIN:"), edtUin);
  edtUin->setValidator(new QIntValidator(10000,2147483647, edtUin));

  // add stretchable space
  grp_lay->setRowMinimumHeight(row, 20);
  grp_lay->setRowStretch(row++, 1);

  ADDLINE(tr("Alias:"), edtNick);
  ADDLINE(tr("First Name:"), edtFirst);
  ADDLINE(tr("Last Name:"), edtLast);
  ADDCMB(tr("Age Range:"), cmbAge, ages);
  ADDCMB(tr("Gender:"), cmbGender, genders);
  ADDCMB(tr("Language:"), cmbLanguage, languages);

  column = 4; // start new column
  row = 2; // skip UIN line

  ADDLINE(tr("City:"), edtCity);
  ADDLINE(tr("State:"), edtState);
  ADDCMB(tr("Country:"), cmbCountry, countries);
  ADDLINE(tr("Company Name:"), edtCoName);
  ADDLINE(tr("Company Department:"), edtCoDept);
  ADDLINE(tr("Company Position:"), edtCoPos);

  ADDFULLLINE(tr("Email Address:"), edtEmail);
  ADDFULLLINE(tr("Keyword:"), edtKeyword);

#undef ADDLINE
#undef ADDFULLLINE
#undef ADDCMB

  chkOnlineOnly = new QCheckBox(tr("Return Online Users Only"));
  grp_lay->addWidget(chkOnlineOnly, row++, 0, 1, 7);

  // Don't let comboboxes grow too much.
  while (!combos.empty())
    combos.takeFirst()->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);

  // post-search widgets
  grpResult = new QGroupBox(tr("Result"));
  grp_lay = new QGridLayout(grpResult);

  foundView = new QTreeWidget();
  QStringList headers = QStringList()
    << tr("Alias") << tr("UIN") << tr("Name") << tr("Email")
    << tr("Status") << tr("A/G") << tr("Auth");
  foundView->setHeaderLabels(headers);
  foundView->setAllColumnsShowFocus(true);
  foundView->setSelectionMode(QTreeWidget::ExtendedSelection);
  foundView->setSortingEnabled(true);
  foundView->sortByColumn(1, Qt::AscendingOrder);
  foundView->setIndentation(0);
  for (int i = 0; i < foundView->columnCount(); i++)
    foundView->resizeColumnToContents(i);
  connect(foundView, SIGNAL(itemSelectionChanged()), SLOT(selectionChanged()));
  grp_lay->addWidget(foundView, 0, 0, 1, 4);

  btnInfo = new QPushButton(tr("View &Info"));
  btnInfo->setEnabled(false);
  connect(btnInfo, SIGNAL(clicked()), SLOT(viewInfo()));
  grp_lay->addWidget(btnInfo, 1, 1);

  btnAdd = new QPushButton(tr("&Add User"));
  btnAdd->setEnabled(false);
  connect(btnAdd, SIGNAL(clicked()), SLOT(addUser()));
  grp_lay->addWidget(btnAdd, 1, 2);

  grp_lay->setRowStretch(0, 1);
  grp_lay->setColumnStretch(0, 1);
  lay->addWidget(grpResult, 1);

  // search-control widgets
  QDialogButtonBox* buttons = new QDialogButtonBox();

  btnSearch = new QPushButton(tr("&Search"), this);
  btnSearch->setDefault(true);
  buttons->addButton(btnSearch, QDialogButtonBox::ActionRole);
  connect(btnSearch, SIGNAL(clicked()), SLOT(startSearch()));

  btnReset = new QPushButton(tr("Reset Search"), this);
  buttons->addButton(btnReset, QDialogButtonBox::ResetRole);
  // dirty hack, needed to ensure the text fits into the button...
  btnReset->setMinimumWidth(btnReset->fontMetrics().width(btnReset->text()) + 20);
  connect(btnReset, SIGNAL(clicked()), SLOT(resetSearch()));

  btnDone = new QPushButton(tr("Close"), this);
  buttons->addButton(btnDone, QDialogButtonBox::RejectRole);
  connect(btnDone, SIGNAL(clicked()), SLOT(close()));

  lay->addWidget(buttons);

  // pseudo Status Bar
  lblSearch = new QLabel(tr("Enter search parameters and select 'Search'"));
  lblSearch->setFrameStyle(QLabel::StyledPanel | QLabel::Sunken);
  lay->addWidget(lblSearch);

  resetSearch();

  show();
}

void SearchUserDlg::startSearch()
{
  unsigned short mins[7] = {0, 18, 23, 30, 40, 50, 60};
  unsigned short maxs[7] = {0, 22, 29, 39, 49, 59, 120};

  foundView->clear();
  for (int i = 0; i < foundView->columnCount(); i++)
    foundView->resizeColumnToContents(i);

  grpParms->hide();
  grpResult->show();

  btnSearch->setEnabled(false);
  btnReset->setText(tr("Cancel"));
  btnDone->setEnabled(false);

  if (edtUin->text().trimmed().isEmpty())
  {
    QTextCodec* codec = QTextCodec::codecForName(gUserManager.DefaultUserEncoding());
    if (codec == 0)
      codec = QTextCodec::codecForLocale();
    searchTag = gLicqDaemon->icqSearchWhitePages(
        codec->fromUnicode(edtFirst->text()),
        codec->fromUnicode(edtLast->text()),
        codec->fromUnicode(edtNick->text()),
        edtEmail->text().toLocal8Bit().data(),
        mins[cmbAge->currentIndex()],
        maxs[cmbAge->currentIndex()],
        cmbGender->currentIndex(),
        GetLanguageByIndex(cmbLanguage->currentIndex())->nCode,
        codec->fromUnicode(edtCity->text()),
        codec->fromUnicode(edtState->text()),
        GetCountryByIndex(cmbCountry->currentIndex())->nCode,
        codec->fromUnicode(edtCoName->text()),
        codec->fromUnicode(edtCoDept->text()),
        codec->fromUnicode(edtCoPos->text()),
        codec->fromUnicode(edtKeyword->text()),
        chkOnlineOnly->isChecked());
  }
  else
    searchTag = gLicqDaemon->icqSearchByUin(edtUin->text().trimmed().toULong());

  lblSearch->setText(tr("Searching (this can take awhile)..."));
}

void SearchUserDlg::resetSearch()
{
  if (searchTag)
  {
    searchTag = 0;
    btnReset->setText(tr("New Search"));
    lblSearch->setText(tr("Search interrupted"));
  }
  else
  {
    if (grpParms->isVisible())
    {
      edtUin->clear();

      edtNick->clear();
      edtFirst->clear();
      edtLast->clear();
      cmbAge->setCurrentIndex(0);
      cmbGender->setCurrentIndex(0);
      cmbLanguage->setCurrentIndex(0);
      edtCity->clear();
      edtState->clear();
      cmbCountry->setCurrentIndex(0);
      edtCoName->clear();
      edtCoDept->clear();
      edtCoPos->clear();
      edtEmail->clear();
      edtKeyword->clear();
      chkOnlineOnly->setChecked(false);
    }
    else
    {
      foundView->clear();
      for (int i = 0; i < foundView->columnCount(); i++)
        foundView->resizeColumnToContents(i);

      grpResult->hide();
      grpParms->show();

      btnReset->setText(tr("Reset Search"));
      lblSearch->setText(tr("Enter search parameters and select 'Search'"));
    }
  }

  btnDone->setEnabled(true);
  btnSearch->setEnabled(true);
}

void SearchUserDlg::searchResult(ICQEvent* e)
{
  if (!e->Equals(searchTag))
    return;

  btnSearch->setEnabled(true);
  btnDone->setEnabled(true);

  if (e->SearchAck() != NULL && e->SearchAck()->Uin() != 0)
    searchFound(e->SearchAck());

  if (e->Result() == EVENT_SUCCESS)
    searchDone(e->SearchAck());
  else if (e->Result() != EVENT_ACKED)
    searchFailed();
}

void SearchUserDlg::searchFound(const CSearchAck* s)
{
  QString text;
  QTreeWidgetItem* item = new QTreeWidgetItem(foundView);
  QTextCodec* codec = QTextCodec::codecForName(gUserManager.DefaultUserEncoding());
  if (codec == NULL)
    codec = QTextCodec::codecForLocale();

  for (int i = 0; i <= 6; i++)
  {
    switch (i)
    {
      case 0:
        item->setData(i, Qt::UserRole, QString::number(s->Uin()));
        text = codec->toUnicode(s->Alias());
        break;
      case 1:
        item->setTextAlignment(i, Qt::AlignRight);
        text = QString::number(s->Uin());
        break;
      case 2:
        text = codec->toUnicode(s->FirstName()) + " " + codec->toUnicode(s->LastName());
        break;
      case 3:
        text = s->Email();
        break;
      case 4:
        switch (s->Status())
        {
          case SA_OFFLINE:
            text = tr("Offline");
            break;
          case SA_ONLINE:
            text = tr("Online");
            break;
          case SA_DISABLED:
          default:
            text = tr("Unknown");
        }
        break;
      case 5:
        text = (s->Age() ? QString::number(s->Age()) : tr("?")) + "/";
        switch (s->Gender())
        {
          case GENDER_FEMALE:
            text += tr("F");
            break;
          case GENDER_MALE:
            text += tr("M");
            break;
          default:
            text += tr("?");
        }
        break;
      case 6:
        text = s->Auth() ? tr("No") : tr("Yes");
        break;
    }
    item->setText(i, text);
  }
}

void SearchUserDlg::searchDone(const CSearchAck* sa)
{
  if (sa == NULL || sa->More() == 0)
    lblSearch->setText(tr("Search complete."));
  else if (sa->More() == ~0UL)
    lblSearch->setText(tr("More users found. Narrow search."));
  else
    lblSearch->setText(tr("%1 more users found. Narrow search.").arg(sa->More()));

  searchTag = 0;
  for (int i = 0; i < foundView->columnCount(); i++)
    foundView->resizeColumnToContents(i);
  btnReset->setText(tr("New Search"));
}

void SearchUserDlg::searchFailed()
{
  searchTag = 0;
  resetSearch();
  lblSearch->setText(tr("Search failed."));
}

void SearchUserDlg::selectionChanged()
{
  int count = foundView->selectedItems().size();

  btnInfo->setEnabled(true);
  btnAdd->setEnabled(true);

  switch (count)
  {
    case 0:
      btnInfo->setEnabled(false);
      btnAdd->setEnabled(false);
      // fall through
    case 1:
      btnAdd->setText(tr("&Add User"));
      break;
    default:
      btnAdd->setText(tr("&Add %1 Users").arg(count));
  }
}

void SearchUserDlg::viewInfo()
{
  foreach (QTreeWidgetItem* current, foundView->selectedItems())
  {
    QByteArray id = current->data(0, Qt::UserRole).toString().toLatin1();

    if (!gUserManager.IsOnList(id, ppid))
      gLicqDaemon->AddUserToList(id, ppid, false, true);

    LicqGui::instance()->showInfoDialog(mnuUserGeneral, id, ppid, false, true);
  }
}

void SearchUserDlg::addUser()
{
  foreach (QTreeWidgetItem* current, foundView->selectedItems())
  {
    QString id = current->data(0, Qt::UserRole).toString();

    new AddUserDlg(id, ppid, this);
  }

  foundView->clearSelection();
}

void SearchUserDlg::reject()
{
  if (searchTag != 0)
    resetSearch();
  else
    QDialog::reject();
}
