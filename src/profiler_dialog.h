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

#ifndef PROFILER_DIALOG_H
#define PROFILER_DIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QGroupBox>
#include <QRadioButton>
#include <QMessageBox>

#include "processor.h"
#include "player.h"
#include "profiler.h"
#include "message_widget.h"
#include "player_panel.h"

class ProfilerDialog : public QDialog
{
  Q_OBJECT

public:
  ProfilerDialog(Processor *prc, Player *plr, PlayerPanel *pnl, QWidget *parent = nullptr);

private:
  Processor *processor;
  Player *player;
  PlayerPanel *playerPanel;

  QComboBox *testSignalComboBox;
  QPushButton *createTestSignalWavButton;
  QLineEdit *responseFileEdit;
  QPushButton *responseFileOpenButton;

  QGroupBox *presetGroupBox;
  QRadioButton *classicPresetRadioButton;
  QRadioButton *mastergainPresetRadioButton;
  QRadioButton *crystalcleanPresetRadioButton;

  QPushButton *analyzeButton;
  QPushButton *cancelButton;

  QString responseFileName;

  ProfilerThread *profilerThread;
  Profiler *profiler;

  MessageWidget *msg;

private slots:
  void analyzeButtonClick();
  void responseFileOpenButtonClick();
  void cancelButtonClick();
  void createTestSignalWavButtonClick();
  void profilerThreadFinished();
  void profilerProgressChanged(int progress);
};

#endif //PROFILER_DIALOG_H
