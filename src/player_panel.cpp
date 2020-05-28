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

#include <QIcon>
#include <QHBoxLayout>
#include <QSettings>

#include "player_panel.h"

PlayerPanel::PlayerPanel(QWidget *parent, Player *plr, Processor *prc) :
  QFrame(parent)
{
  player = plr;

  connect(player, &Player::dataChanged, this, &PlayerPanel::playerDataChanged);
  connect(player, &Player::equalRMSFinished, this, &PlayerPanel::playerEqualRMSFinished);

  processor = prc;
  setFrameShape(QFrame::Panel);

  playReferenceTrack = false;
  playPaused = true;

  QHBoxLayout *hbox = new QHBoxLayout(this);

  buttonLoad = new QPushButton(this);
  buttonLoad->setToolTip(tr("Load DI and Reference files"));
  hbox->addWidget(buttonLoad);
  buttonLoad->setIcon(QIcon(":/icons/load.png"));
  buttonLoad->setIconSize(QSize(24, 24));

  connect(buttonLoad, &QPushButton::clicked, this, &PlayerPanel::loadButtonClicked);

  buttonStop = new QPushButton(this);
  buttonStop->setToolTip(tr("Stop playback"));
  hbox->addWidget(buttonStop);
  buttonStop->setIcon(QIcon(":/icons/stop.png"));
  buttonStop->setIconSize(QSize(24, 24));

  connect(buttonStop, &QPushButton::clicked, this, &PlayerPanel::buttonStopClicked);

  buttonPlay = new QPushButton(this);
  buttonPlay->setToolTip(tr("Start playback"));
  hbox->addWidget(buttonPlay);
  buttonPlay->setIcon(QIcon(":/icons/play.png"));
  buttonPlay->setIconSize(QSize(24, 24));

  connect(buttonPlay, &QPushButton::clicked, this, &PlayerPanel::buttonPlayClicked);

  buttonMonitor = new QPushButton(this);
  buttonMonitor->setToolTip(tr("Enable Monitor Mode"));
  hbox->addWidget(buttonMonitor);
  buttonMonitor->setIcon(QIcon(":/icons/monitor.png"));
  buttonMonitor->setIconSize(QSize(24, 24));
  buttonMonitor->setCheckable(true);

  connect(buttonMonitor, &QPushButton::clicked, this, &PlayerPanel::buttonMonitorClicked);

  diButton = new QPushButton(tr("DI"), this);
  hbox->addWidget(diButton);

  connect(diButton, &QPushButton::clicked, this, &PlayerPanel::diButtonClicked);

  equalRMSButton = new QPushButton(this);
  equalRMSButton->setToolTip(tr("Adjust Reference file volume "
    "to match amplifier output volume"));
  equalRMSButton->setIcon(QIcon(":/icons/equalRMS.png"));
  equalRMSButton->setIconSize(QSize(24, 24));
  hbox->addWidget(equalRMSButton);

  connect(equalRMSButton, &QPushButton::clicked, this,
          &PlayerPanel::equalRMSButtonClicked);

  inputLevelSlider = new QSlider(Qt::Horizontal, this);
  inputLevelSlider->setMinimumWidth(200);
  inputLevelSlider->setRange(0, 100);
  inputLevelSlider->setSingleStep(1);
  inputLevelSlider->setValue(50);
  inputLevelSlider->setToolTip(tr("Input Level"));
  hbox->addWidget(inputLevelSlider);

  connect(inputLevelSlider, &QSlider::valueChanged,
          this, &PlayerPanel::sliderValueChanged);

  QSettings settings;
  if (settings.contains("playerPanel/inputLevel"))
  {
    int value = settings.value("playerPanel/inputLevel").toInt();
    player->setInputLevel(value);
    inputLevelSlider->setValue(value + 50);
  }

  loadDialog = new LoadDialog(this);

  connect(loadDialog, &QDialog::finished, this, &PlayerPanel::loadDialogFinished);

  diFileResamplingThread = new FileResamplingThread();
  refFileResamplingThread = new FileResamplingThread();

  diFileResamplingThreadWorking = false;
  refFileResamplingThreadWorking = false;

  connect(diFileResamplingThread, &QThread::finished, this,
   &PlayerPanel::diFileResamplingThreadFinished);

  connect(refFileResamplingThread, &QThread::finished, this,
   &PlayerPanel::refFileResamplingThreadFinished);

  buttonPlay->setEnabled(false);
  buttonStop->setEnabled(false);
  diButton->setEnabled(false);

  msg = new MessageWidget(this);
}

void PlayerPanel::diButtonClicked()
{
  if (playReferenceTrack)
  {
    diButton->setText(tr("DI"));
    playReferenceTrack = false;
    if (player->status == Player::PlayerStatus::PS_PLAY_REF)
    {
      player->setStatus(Player::PlayerStatus::PS_PLAY_DI);
    }
  }
  else
  {
    diButton->setText(tr("Ref."));
    playReferenceTrack = true;
    if (player->status == Player::PlayerStatus::PS_PLAY_DI)
    {
      player->setStatus(Player::PlayerStatus::PS_PLAY_REF);
    }
  }
}

void PlayerPanel::loadButtonClicked()
{
  player->setStatus(Player::PlayerStatus::PS_STOP);
  playPaused = true;
  loadDialog->open();
}

void PlayerPanel::loadDialogFinished(int result)
{
  if (result == QDialog::Accepted)
  {
    QString diFilename = loadDialog->getDiFileName();
    QString refFilename = loadDialog->getRefFileName();

    msg->setTitle(tr("Please wait!"));
    msg->setMessage(tr("Resampling files..."));
    msg->setProgressValue(0);
    msg->open();

    diFileResamplingThread->filename = diFilename;
    refFileResamplingThread->filename = refFilename;
    diFileResamplingThread->samplingRate = processor->getSamplingRate();
    refFileResamplingThread->samplingRate = processor->getSamplingRate();

    diFileResamplingThreadWorking = true;
    refFileResamplingThreadWorking = true;

    refFileResamplingThread->stereoMode = true;

    diFileResamplingThread->start();
    refFileResamplingThread->start();
  }
}

void PlayerPanel::buttonPlayClicked()
{
  if (!playPaused)
  {
    buttonPlay->setIcon(QIcon(":/icons/pause.png"));
    player->setStatus(Player::PlayerStatus::PS_PAUSE);
    playPaused = true;
  }
  else
  {
    buttonPlay->setIcon(QIcon(":/icons/play.png"));
    if (playReferenceTrack)
    {
      player->setStatus(Player::PlayerStatus::PS_PLAY_REF);
    }
    else
    {
      player->setStatus(Player::PlayerStatus::PS_PLAY_DI);
    }
    playPaused = false;
  }
}

void PlayerPanel::buttonStopClicked()
{
  player->setStatus(Player::PlayerStatus::PS_STOP);
  buttonPlay->setIcon(QIcon(":/icons/play.png"));
  playPaused = true;
}

void PlayerPanel::buttonMonitorClicked()
{
  if (player->status != Player::PlayerStatus::PS_MONITOR)
  {
    player->setStatus(Player::PlayerStatus::PS_MONITOR);
    buttonPlay->setEnabled(false);
    buttonStop->setEnabled(false);
    diButton->setEnabled(false);
  }
  else
  {
    player->setStatus(Player::PlayerStatus::PS_STOP);
    if (player->diData.size() != 0)
    {
      buttonPlay->setEnabled(true);
      buttonStop->setEnabled(true);
      diButton->setEnabled(true);
    }
  }
}

void PlayerPanel::diFileResamplingThreadFinished()
{
  player->setDiData(diFileResamplingThread->dataL);

  diFileResamplingThreadWorking = false;

  if ((!diFileResamplingThreadWorking) && (!refFileResamplingThreadWorking))
  {
    resamplingFinished();
  }
  else
  {
    msg->setProgressValue(10);
  }
}

void PlayerPanel::refFileResamplingThreadFinished()
{
  player->setRefData(refFileResamplingThread->dataL, refFileResamplingThread->dataR);

  refFileResamplingThreadWorking = false;

  if ((!diFileResamplingThreadWorking) && (!refFileResamplingThreadWorking))
  {
    resamplingFinished();
  }
  else
  {
    msg->setProgressValue(50);
  }
}

void PlayerPanel::resamplingFinished()
{
  msg->setProgressValue(100);
  msg->close();
}

void PlayerPanel::playerDataChanged()
{
  if (player->diData.size() != 0)
  {
    buttonPlay->setEnabled(true);
    buttonStop->setEnabled(true);
    diButton->setEnabled(true);
  }
}

void PlayerPanel::equalRMSButtonClicked()
{
  player->equalDataRMS();

  if (player->isEqualDataRMSThreadRunning)
  {
    msg->setTitle(tr("Please wait!"));
    msg->setMessage(tr("Adjusting Reference sound volume..."));
    msg->setProgressValue(0);
    msg->open();
  }
}

void PlayerPanel::sliderValueChanged(int value)
{
  player->setInputLevel(value - 50);
}

int PlayerPanel::getInputLevelSliderValue()
{
  return inputLevelSlider->value();
}

void PlayerPanel::playerEqualRMSFinished()
{
  msg->close();
}

void PlayerPanel::stopPlayback()
{
  buttonStopClicked();
}
