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

#ifndef CONVOLVERDIALOG_H
#define CONVOLVERDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QLineEdit>
#include <QRadioButton>
#include <QButtonGroup>

#include "processor.h"

class ConvolverDialog : public QDialog
{
  Q_OBJECT

public:
  ConvolverDialog(Processor *prc, QWidget *parent = nullptr);

private:
  Processor *processor;
  QPushButton *processButton;

  QLineEdit *inputFilenameEdit;
  QLineEdit *outputFilenameEdit;
  QLineEdit *IRFilenameEdit;

  QPushButton *outputFilenameButton;
  QPushButton *inputFilenameButton;

  QRadioButton *inputCabinetRadioButton;
  QRadioButton *inputFileRadioButton;

  QRadioButton *outputCabinetRadioButton;
  QRadioButton *outputFileRadioButton;

  void checkSignals();

public slots:
  void inputFilenameButtonClicked();
  void outputFilenameButtonClicked();
  void IRFilenameButtonClicked();
  void closeButtonClicked();
  void processButtonClicked();

  void outputGroupClicked(QAbstractButton *button);
  void inputGroupClicked(QAbstractButton *button);
};

#endif // CONVOLVERDIALOG_H
