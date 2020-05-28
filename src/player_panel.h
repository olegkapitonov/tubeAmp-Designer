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

#ifndef PLAYERPANEL_H
#define PLAYERPANEL_H

#include <QFrame>
#include <QPushButton>
#include <QMessageBox>
#include <QSlider>

#include "load_dialog.h"
#include "player.h"
#include "file_resampling_thread.h"
#include "processor.h"
#include "message_widget.h"

class PlayerPanel : public QFrame
{
  Q_OBJECT

public:
  PlayerPanel(QWidget *parent = nullptr, Player *plr = nullptr, Processor *prc = nullptr);
  int getInputLevelSliderValue();

private:
  bool diFileResamplingThreadWorking;
  bool refFileResamplingThreadWorking;

  QPushButton *buttonStop;
  QPushButton *buttonPlay;
  QPushButton *buttonMonitor;
  QPushButton *buttonLoad;
  QPushButton *diButton;
  QPushButton *equalRMSButton;

  QSlider *inputLevelSlider;

  LoadDialog *loadDialog;

  bool playReferenceTrack;
  bool playPaused;

  Player *player;
  Processor *processor;
  MessageWidget *msg;
  FileResamplingThread *diFileResamplingThread;
  FileResamplingThread *refFileResamplingThread;

  void resamplingFinished();

public slots:
  void diButtonClicked();
  void loadButtonClicked();
  void loadDialogFinished(int result);

  void buttonPlayClicked();
  void buttonStopClicked();
  void buttonMonitorClicked();
  void equalRMSButtonClicked();

  void diFileResamplingThreadFinished();
  void refFileResamplingThreadFinished();

  void playerDataChanged();

  void sliderValueChanged(int value);
  void playerEqualRMSFinished();

  void stopPlayback();
};

#endif // PLAYERPANEL_H
