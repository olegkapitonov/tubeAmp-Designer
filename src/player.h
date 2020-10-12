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

#ifndef PLAYER_H
#define PLAYER_H

#include <QVector>

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <portaudio.h>

#include "processor.h"

class Player;

class EqualDataRMSThread : public QThread
{
  Q_OBJECT

  void run() override;

public:
  Player *player;
  Processor *processor;
};

class Player : public QObject
{
  Q_OBJECT

public:

  Player();
  ~Player();

  PaStreamParameters inputParameters, outputParameters;
  PaStream *stream;
  PaError err;

  int simple_quit;

  void setProcessor(Processor *prc);
  int connectToPortAudio();
  int activate();

  void setLevel(float lev);
  float getLevel();

  void setDiData(QVector<float> data);
  void setRefData(QVector<float> dataL, QVector<float> dataR);

  void equalDataRMS();

  QVector<float> diData;
  QVector<float> refDataL;
  QVector<float> refDataR;

  unsigned int diPos;
  unsigned int refPos;

  enum PlayerStatus{
    PS_STOP,
    PS_PAUSE,
    PS_PLAY_DI,
    PS_PLAY_REF,
    PS_MONITOR
  };

  PlayerStatus status;

  Processor *processor;

  void setStatus(PlayerStatus newStatus);
  int getSampleRate();
  void incRMScounter();

  void setInputLevel(float dbInputLevel);

  float inputLevel = 1.0;
  bool isEqualDataRMSThreadRunning = false;

private:
  int sampleRate;
  EqualDataRMSThread *equalDataRMSThread;

  float level = 1.0;

private slots:
  void equalDataRMSThreadFinished();

signals:
  void dataChanged();
  void peakRMSValueCalculated(float inputValue, float outputValue);
  void equalRMSFinished();
};

#endif //PLAYER_H
