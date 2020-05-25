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

#ifndef DECONVOLVERDIALOG_H
#define DECONVOLVERDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QLineEdit>
#include <QRadioButton>
#include <QButtonGroup>

#include "processor.h"

class DeconvolverDialog : public QDialog
{
  Q_OBJECT

public:
  DeconvolverDialog(Processor *prc, QWidget *parent = nullptr);

private:
  Processor *processor;
  QPushButton *processButton;

  QLineEdit *testFilenameEdit;
  QLineEdit *responseFilenameEdit;
  QLineEdit *IRFilenameEdit;

  QPushButton *testFilenameButton;
  QPushButton *responseFilenameButton;
  QPushButton *IRFilenameButton;

  QRadioButton *IRCabinetRadioButton;
  QRadioButton *IRFileRadioButton;

  void checkSignals();

public slots:
  void testFilenameButtonClicked();
  void responseFilenameButtonClicked();
  void IRFilenameButtonClicked();
  void closeButtonClicked();
  void processButtonClicked();
  void saveTestSignalButtonClicked();

  void IRGroupClicked(QAbstractButton *button);
};

#endif // DECONVOLVERDIALOG_H
