/*
 * This file is part of Licq, an instant messaging client for UNIX.
 * Copyright (C) 2004-2009 Licq developers
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

#ifndef CATDLG_H
#define CATDLG_H

#include <qdialog.h>

#include "licq_user.h"

class QPushButton;
class QComboBox;
class QLineEdit;

class EditCategoryDlg : public QDialog
{
  Q_OBJECT
public:
  EditCategoryDlg(QWidget* parent, UserCat cat, const UserCategoryMap& category);

protected:
  QComboBox* cbCat[MAX_CATEGORIES];
  QLineEdit* leDescr[MAX_CATEGORIES];
  const struct SCategory *(*m_fGetEntry)(unsigned short);
  UserCat myUserCat;
  unsigned short m_nCats;
signals:
  void updated(UserCat cat, const UserCategoryMap& category);

protected slots:
  void ok();
  void checkEnabled(int index);
};

#endif