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

#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <stdint.h>
#include <QVector>
#include <QString>
#include <QThread>

#include "profile.h"

#include <zita-convolver.h>

// Defines for compatability with
// FAUST generated code

#define VOLUME_CTRL controls.volume
#define DRIVE_CTRL controls.drive
#define MASTERGAIN_CTRL controls.mastergain

#define AMP_BIAS_CTRL profile->amp_bias
#define AMP_KREG_CTRL profile->amp_Kreg
#define AMP_UPOR_CTRL profile->amp_Upor

#define PREAMP_BIAS_CTRL profile->preamp_bias
#define PREAMP_KREG_CTRL profile->preamp_Kreg
#define PREAMP_UPOR_CTRL profile->preamp_Upor

#define LOW_CTRL controls.low
#define MIDDLE_CTRL controls.middle
#define HIGH_CTRL controls.high

#define LOW_FREQ_CTRL profile->tonestack_low_freq
#define MIDDLE_FREQ_CTRL profile->tonestack_middle_freq
#define HIGH_FREQ_CTRL profile->tonestack_high_freq

#define LOW_BAND_CTRL profile->tonestack_low_band
#define MIDDLE_BAND_CTRL profile->tonestack_middle_band
#define HIGH_BAND_CTRL profile->tonestack_high_band

#define PREAMP_LEVEL profile->preamp_level
#define AMP_LEVEL profile->amp_level

#define SAG_TIME profile->sag_time
#define SAG_COEFF profile->sag_coeff

#define OUTPUT_LEVEL profile->output_level

// Zita-convolver parameters
#define CONVPROC_SCHEDULER_PRIORITY 0
#define CONVPROC_SCHEDULER_CLASS SCHED_FIFO
#define THREAD_SYNC_MODE true

#define fragm 64

struct stControls
{
  float volume;
  float drive;
  float low;
  float middle;
  float high;
  float mastergain;
};

using namespace std;
#include "faust-support.h"

class mydsp;

class ConvolverDeleteThread : public QThread
{
  Q_OBJECT

  void run() override;

public:
  Convproc *convolver;
};

class Processor
{
public:
  Processor(int SR);
  ~Processor();

  bool loadProfile(QString filename);
  bool saveProfile(QString filename);

  QVector<float> getPreampFrequencyResponse(QVector<float> freqs);
  QVector<float> getCabinetSumFrequencyResponse(QVector<float> freqs);

  QVector<double> preampCorrectionEqualizerFLogValues;
  QVector<double> preampCorrectionEqualizerDbValues;

  QVector<double> correctionEqualizerFLogValues;
  QVector<double> correctionEqualizerDbValues;

  stControls getControls();
  void setControls(stControls newControls);
  st_profile getProfile();
  void setProfile(st_profile newProfile);

  float tube(float Uin, float Kreg, float Upor, float bias, float cut);

  void setPreampCorrectionImpulseFromFrequencyResponse(QVector<double> w, QVector<double> A);
  void setCabinetSumCorrectionImpulseFromFrequencyResponse(QVector<double> w, QVector<double> A);
  void applyPreampCorrection();
  void applyCabinetSumCorrection();

  void resetPreampCorrection();
  void resetCabinetSumCorrection();
  int getSamplingRate();

  void process(float *outL, float *outR, float *in, int nSamples);

  QString getProfileFileName();
  void setProfileFileName(QString name);
  bool isPreampCorrectionEnabled();
  bool isCabinetCorrectionEnabled();

  void cleanProfile();

  void setPreampImpulse(QVector<float> data);
  void setCabinetImpulse(QVector<float> dataL, QVector<float> dataR);

  void setPreampCorrectionStatus(bool status);
  void setCabinetCorrectionStatus(bool status);

  QVector<float> getPreampImpulse();
  QVector<float> getLeftImpulse();
  QVector<float> getRightImpulse();

private:
  Convproc *preamp_convproc;
  Convproc *preamp_correction_convproc;
  Convproc *convproc;
  Convproc *correction_convproc;

  Convproc *new_preamp_convproc;
  Convproc *new_preamp_correction_convproc;
  Convproc *new_convproc;
  Convproc *new_correction_convproc;

  QVector<float> preamp_impulse;
  QVector<float> left_impulse;
  QVector<float> right_impulse;

  QVector<float> preamp_correction_impulse;
  QVector<float> left_correction_impulse;
  QVector<float> right_correction_impulse;

  bool preampCorrectionEnabled;
  bool cabinetCorrectionEnabled;

  ConvolverDeleteThread *convolverDeleteThread;

  QString currentProfileFile;
  int samplingRate;

  mydsp *dsp;

  QString profileFileName;

  void freeConvolver(Convproc *convolver);
  int checkProfileFile(const char *path);

  QVector<float> getFrequencyResponse(QVector<float> freqs, QVector<float> impulse);
  Convproc* createMonoConvolver(QVector<float> impulse);
  Convproc* createStereoConvolver(QVector<float> left_impulse, QVector<float> right_impulse);
};

#endif //PROCESSOR_H
