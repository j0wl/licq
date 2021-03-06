/*
 * This file is part of Licq, an instant messaging client for UNIX.
 * Copyright (C) 2007-2012 Licq developers <licq-dev@googlegroups.com>
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

#include "settingsdlg.h"

#include "config.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "core/licqgui.h"
#include "core/mainwin.h"

#include "helpers/support.h"

#include "widgets/treepager.h"

#include "chat.h"
#include "contactlist.h"
#include "events.h"
#include "general.h"
#include "network.h"
#include "plugins.h"
#include "shortcuts.h"
#include "skin.h"
#include "status.h"

using namespace LicqQtGui;
/* TRANSLATOR LicqQtGui::SettingsDlg */

SettingsDlg* SettingsDlg::myInstance = NULL;

void SettingsDlg::show(SettingsPage page)
{
  if (myInstance == NULL)
    myInstance = new SettingsDlg(gMainWindow);

  myInstance->showPage(page);
  myInstance->raise();
}

SettingsDlg::SettingsDlg(QWidget* parent)
  : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
{
  Support::setWidgetProps(this, "SettingsDialog");
  setWindowTitle(tr("Licq - Settings"));
  setAttribute(Qt::WA_DeleteOnClose, true);

  QVBoxLayout* top_lay = new QVBoxLayout(this);

  myPager = new TreePager(this);
  top_lay->addWidget(myPager);

  QDialogButtonBox* buttons = new QDialogButtonBox(
      QDialogButtonBox::Ok |
      QDialogButtonBox::Cancel |
      QDialogButtonBox::Apply);

  connect(buttons, SIGNAL(accepted()), SLOT(ok()));
  connect(buttons, SIGNAL(rejected()), SLOT(close()));
  connect(buttons->button(QDialogButtonBox::Apply),
      SIGNAL(clicked()), SLOT(apply()));

  top_lay->addWidget(buttons);

  myContactListSettings = new Settings::ContactList(this);
  myGeneralSettings = new Settings::General(this);
  myChatSettings = new Settings::Chat(this);
  myEventsSettings = new Settings::Events(this);
  myNetworkSettings = new Settings::Network(this);
  mySkinSettings = new Settings::Skin(this);
  myStatusSettings = new Settings::Status(this);
  myShortcutsSettings = new Settings::Shortcuts(this);
  myPluginsSettings = new Settings::Plugins(this);

  QDialog::show();
}

SettingsDlg::~SettingsDlg()
{
  myInstance = NULL;
}

void SettingsDlg::addPage(SettingsPage page, QWidget* widget, const QString& title, SettingsPage parent)
{
  myPages.insert(page, widget);
  myPager->addPage(widget, title, (parent == UnknownPage ? NULL : myPages.value(parent)));
}

void SettingsDlg::showPage(SettingsPage page)
{
  if (myPages.contains(page))
    myPager->showPage(myPages.value(page));
}

void SettingsDlg::ok()
{
  apply();
  gLicqGui->saveConfig();
  close();
}

void SettingsDlg::apply()
{
  myGeneralSettings->apply();
  myContactListSettings->apply();
  myChatSettings->apply();
  myEventsSettings->apply();
  myNetworkSettings->apply();
  myShortcutsSettings->apply();
  mySkinSettings->apply();
  myStatusSettings->apply();
}
