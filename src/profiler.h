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

#ifndef PROFILER_H
#define PROFILER_H

#include <QString>
#include <QVector>
#include <QThread>

#include "processor.h"
#include "player.h"

enum ProfilerPresetType {CRYSTALCLEAN_PRESET, CLASSIC_PRESET, MASTERGAIN_PRESET};

struct stCrunchPoint
{
  double max;
  double rmsAtMax;
  int maxtime;
  bool isBad;
};

struct sPike
{
  double value;
  int time;
};

class Profiler : public QObject
{
  Q_OBJECT

public:
  Profiler(Processor *prc, Player *plr);
  void loadResponseFile(QString fileName);
  void createTestFile(QString fileName, int version);
  void analyze(ProfilerPresetType preset);

private:
  Processor *processor;
  Player *player;

  int responseDataSamplerate;
  int responseDataChannels;
  QVector<float> responseData;

  stCrunchPoint findCrunchPoint(int freqIndex, QVector<float> data, int samplerate);
  QVector<float> loadRealTestFile(QVector<float> testSignal, float sampleRate);

  void createTestFile_v1(QString fileName);

private slots:
  void warningMessageNeededSlot(QString message);

signals:
  void warningMessageNeeded(QString message);
  void progressChanged(int progress);
  void stopPlaybackNeeded();
};

class ProfilerThread : public QThread
{
  Q_OBJECT

  void run() override;

public:
  Profiler *profiler;
  ProfilerPresetType presetType;
};

#endif //PROFILER_H
