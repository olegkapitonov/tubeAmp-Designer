/*
 * Copyright (C) 2018-2020 Oleg Kapitonov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * --------------------------------------------------------------------------
 */

#ifndef LOADDIALOG_H
#define LOADDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QLineEdit>

class LoadDialog : public QDialog
{
  Q_OBJECT

public:
  LoadDialog(QWidget *parent = nullptr);

  QString getDiFileName();
  QString getRefFileName();

private:
  QString diFileName;
  QString refFileName;

  QPushButton *okButton;

  QLineEdit *diFilenameEdit;
  QLineEdit *refFilenameEdit;

  void checkFilenames();

public slots:
  void diFilenameButtonClicked();
  void refFilenameButtonClicked();
  void cancelButtonClicked();
  void okButtonClicked();
  void lineEditEdited(QString);
};

#endif // LOADDIALOG_H
