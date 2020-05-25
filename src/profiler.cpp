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

#include <QSharedPointer>

#include <sndfile.h>
#include <cmath>
#include <cfloat>
#include <QMessageBox>
#include <QDir>
#include <QApplication>

#include "profiler.h"
#include "math_functions.h"

#define EXPER_POINTS_NUM 15
#define TEST_SIGNAL_LENGTH_SEC 3.0
#define TEST_SIGNAL_PAUSE_LENGTH_SEC 0.1
#define RESPONSE_OVERSAMPLING_COEFF 16

// Frequencies used to generate test signal
// to get preamp amplitude response, in rad/s
int calibration_freqs[] = {200,600,1500,2000,2500,3000,3500,
  4000,4500,5000,12000,25000,35000,50000,80000};

// Amplitude values for each frequency in test signal
// Different amplitude values are used
// to extend dynamic range
double freq_weights[] = {1.0,0.8,0.75,0.6,0.55,0.5,0.5,
  0.4,0.35,0.3,0.25,0.2,0.35,0.75,1.0};

Profiler::Profiler(Processor *prc, Player *plr)
{
  processor = prc;
  player = plr;

  connect(this, &Profiler::warningMessageNeeded, this,
          &Profiler::warningMessageNeededSlot);
}

void Profiler::loadResponseFile(QString fileName)
{
  responseData.resize(0);

  SF_INFO sfinfo;
  sfinfo.format = 0;

  SNDFILE *responseSndFile = sf_open(fileName.toUtf8(), SFM_READ, &sfinfo);
  if (responseSndFile != NULL)
  {
    responseDataSamplerate = sfinfo.samplerate;
    responseDataChannels = sfinfo.channels;

    responseData.resize(sfinfo.frames * sfinfo.channels);
    sf_readf_float(responseSndFile, responseData.data(), sfinfo.frames);

    sf_close(responseSndFile);
  }
}

// Real test file contains DI from guitar
QVector<float> Profiler::loadRealTestFile(QVector<float> testSignal, float sampleRate)
{
  SF_INFO sfinfo;
  sfinfo.format = 0;

  int realTestDataSamplerate;

  QDir profilesDir(QCoreApplication::applicationDirPath());
  profilesDir.cdUp();
  profilesDir.cd("share/tubeAmp Designer");
  SNDFILE *realTestSndFile = sf_open(QString(profilesDir.absolutePath() +
                                     "/real_test.wav").toUtf8(),
                                     SFM_READ, &sfinfo);
  if (realTestSndFile != NULL)
  {
    realTestDataSamplerate = sfinfo.samplerate;

    if (realTestDataSamplerate == sampleRate)
    {
      int startRealTest = testSignal.length();
      testSignal.resize(testSignal.length() + sfinfo.frames * sfinfo.channels);
      sf_readf_float(realTestSndFile, &(testSignal.data()[startRealTest]), sfinfo.frames);
    }
    else
    {
      QVector<float> tempTestSignal(sfinfo.frames * sfinfo.channels);
      sf_readf_float(realTestSndFile, tempTestSignal.data(), sfinfo.frames);

      QVector<float> tempTestSignalResampled = resample_vector(tempTestSignal,
                                                               realTestDataSamplerate,
                                                               sampleRate);

      int startRealTest = testSignal.size();
      testSignal.resize(testSignal.size() + tempTestSignalResampled.size());

      for (int i = 0; i < tempTestSignalResampled.size(); i++)
      {
        testSignal[i + startRealTest] = tempTestSignalResampled[i];
      }
    }

    sf_close(realTestSndFile);
  }
  return testSignal;
}

// Calculates amplitude of the test signal
// at which profiled amplifier starts to clip
stCrunchPoint Profiler::findCrunchPoint(int freqIndex,
                                          QVector<float> data,
                                          int samplerate)
{
    int periodInSamples = samplerate /
      ((double)calibration_freqs[freqIndex] /
      2.0 / M_PI);

    int periodsNum = (double)samplerate *
      (double)TEST_SIGNAL_LENGTH_SEC /
      periodInSamples;

    // Calculate RMS values of each period
    // of the test signal
    QVector<sPike> pikeArray;

    for (int j = 0; j < periodsNum - 2; j++)
    {
      double rmsSum = 0.0;
      for (int i = 0; i < periodInSamples; i++)
      {
        rmsSum += pow(data[i + j * periodInSamples], 2);
      }

      double rms = sqrt(sqrt(rmsSum / periodInSamples));

      sPike pikeElement;
      pikeElement.value = rms;
      pikeElement.time = j * periodInSamples + periodInSamples / 2;

      pikeArray.append(pikeElement);
    }

    // Compare initial form of the test signal
    // and signal form after the profiled amplifier (response form).
    // At point in time when profiled amplifier starts to clip
    // the difference between test and response form
    // will be maximal. Find this point in time here.
    double stepCoeff = (double)pikeArray[pikeArray.size() - 1].value /
      pow((double)pikeArray[pikeArray.size() - 1].time, 1);

    stCrunchPoint crunchPoint;
    crunchPoint.max = 0.0;
    crunchPoint.maxtime = 0;

    for (int i = 0; i < pikeArray.size() - 1; i++)
    {
      double errorValue = fabs(pikeArray[i].value - pow(pikeArray[i].time, 1) *
                               stepCoeff);

      if (errorValue > crunchPoint.max)
      {
          crunchPoint.max = errorValue;
          crunchPoint.maxtime = pikeArray[i].time;
          crunchPoint.rmsAtMax = pikeArray[i].value;
      }
    }

    // Mark point as Bad if the clipping was not reached
    // Exclude last two high frequency points from this (hack!)
    if ((fabs(crunchPoint.rmsAtMax - pikeArray[pikeArray.size() - 1].value) /
      pikeArray[pikeArray.size() - 1].value > 0.2) && (freqIndex != 0) &&
        (freqIndex != 13) && (freqIndex != 14))
    {
      crunchPoint.isBad = true;
    }
    else
    {
      crunchPoint.isBad = false;
    }

    return crunchPoint;
}

// Caclulates all tubeAmp profile components
// by analyzing response signal from profiled amplifier
void Profiler::analyze(ProfilerPresetType preset)
{
  if (responseData.size() < (int)((3285485.0 * (double)responseDataSamplerate) / 44100.0 + 1))
  {
    QMessageBox::critical(nullptr, QObject::tr("Error!"),
                         tr("Response file is too short!"),
                         QMessageBox::Ok,
                         QMessageBox::Ok);
    return;
  }

  // Remove first 1 second from response signal.
  responseData.remove(0, 1 * responseDataSamplerate * responseDataChannels);

  QVector<float> preamp_impulse(0.1 * processor->getSamplingRate());
  double desiredGain;

  if (preset == CRYSTALCLEAN_PRESET)
  {
    QVector<double> A({
                        0.1,
                        0.5,
                        0.75,
                        1.0,
                        1.0,
                        1.0,
                        1.0,
                        0.8,
                        0.5,
                        0.1
                      });
    QVector<double> w({
                        125,
                        314,
                        628,
                        1256,
                        3141,
                        6283,
                        12566,
                        25132,
                        31415,
                        62831
                      });

    // Calculate impulse response (for convolver in tubeAmp)
    // of the preamp
    frequency_response_to_impulse_response(w.data(),
                                            A.data(),
                                            w.size(),
                                            preamp_impulse.data(),
                                            preamp_impulse.size(),
                                            processor->getSamplingRate());

    desiredGain = 0.0005;

    emit progressChanged(50);
  }
  else
  {
    int responseDataSamplerateOversampled = responseDataSamplerate *
      RESPONSE_OVERSAMPLING_COEFF;

    int crunchTestSize = ((TEST_SIGNAL_LENGTH_SEC + TEST_SIGNAL_PAUSE_LENGTH_SEC) *
      EXPER_POINTS_NUM + 1) * responseDataSamplerate * responseDataChannels;

    QVector<float> tempResponseData;
    for (int i = 0; i < crunchTestSize; i += responseDataChannels)
    {
      tempResponseData.append(responseData[i]);
    }

    // 1. Process "crunch" test part of the response singal.
    //    This part contains 14 test signals with different frequencies.
    //    For each frequency we will caclulate amplitudes of input signal
    //    at which profiled amplifier starts to clip.
    //    As a result we will get amplitude response of the preamp.

    // Oversample crunch test part for better accuracy
    QVector<float> responseDataOversampled = resample_vector(tempResponseData,
                                                      responseDataSamplerate,
                                                      responseDataSamplerateOversampled);

    emit progressChanged(20);

    stCrunchPoint crunchPoint;

    // Calculate amplitude response of the preamp
    QVector<double> Aexper(EXPER_POINTS_NUM);
    QVector<double> wexper(EXPER_POINTS_NUM);

    // Holds maximum amplitude value of the response
    // for normalization
    double Amax = 0.0;

    int badPointsCounter = 0;

    for (int j = 0; j < EXPER_POINTS_NUM; j++)
    {
      QVector<float> A(responseDataOversampled.size());

      for (int i = 0; i < responseDataSamplerateOversampled *
        TEST_SIGNAL_LENGTH_SEC - 1; i++)
      {
        A[i] = responseDataOversampled[i +
          j * (responseDataSamplerateOversampled *
          TEST_SIGNAL_LENGTH_SEC +
          responseDataSamplerateOversampled *
          TEST_SIGNAL_PAUSE_LENGTH_SEC)];
      }

      crunchPoint = findCrunchPoint(j, A, responseDataSamplerateOversampled);
      if (crunchPoint.isBad)
      {
        badPointsCounter++;
      }

      wexper[j] = calibration_freqs[j];
      Aexper[j] = (double)responseDataSamplerateOversampled *
        (double)TEST_SIGNAL_LENGTH_SEC / crunchPoint.maxtime;

      Aexper[j] = Aexper[j] * Aexper[j] / freq_weights[j];

      if (Aexper[j]>Amax)
      {
        Amax = Aexper[j];
      }
    }

    // Calculate Gain level of profiled amplifier
    desiredGain = Amax / responseDataSamplerate * 7.0;

    // Check if the gain is within a reasonable range
    if (desiredGain < 0.0001)
    {
      QString message = QString(QObject::tr("Response file was created with too"
                       " low gain,\n"
                       "for adequate results please increase the gain\n"
                       "by at least %1 db!")).arg(20.0 * log10(0.001 / desiredGain),
                                                  4, 'f', 2);
      emit warningMessageNeeded(message);
    }
    else if (badPointsCounter != 0)
    {
      QString message = QString(QObject::tr("Response file was created with too"
                       " low gain,\n"
                       "for adequate results please increase the gain\n"
                       "by 10 db!"));

      emit warningMessageNeeded(message);
    }

    if (desiredGain > 0.2)
    {
      QString message = QString(QObject::tr("Response file was created with too"
                       " high gain,\n"
                       "for adequate results please decrease the gain\n"
                       "by at least %1 db!")).arg(20.0*log10(desiredGain / 0.2),4,'f',2);

      emit warningMessageNeeded(message);
    }

    // Limit the gain to absolute maximum and minimum level
    if (desiredGain > 3.16 * 10.0)
    {
      desiredGain = 3.16 * 10.0;
    }

    if (desiredGain < 0.0005)
    {
      desiredGain = 0.0005;
    }

    // Perform normalization of the preamp amplitude response
    for (int i = 0; i < EXPER_POINTS_NUM; i++)
    {
      Aexper[i] /= Amax;
      if (Aexper[i] < 0.01)
      {
        Aexper[i] = 0.01;
      }
    }

    // Calculate impulse response (for convolver in tubeAmp)
    // of the preamp
    frequency_response_to_impulse_response(wexper.data(),
                                            Aexper.data(),
                                            wexper.size(),
                                            preamp_impulse.data(),
                                            preamp_impulse.size(),
                                            processor->getSamplingRate());

    emit progressChanged(50);
  }

  // 2. Calculate frequency response of the part after clipping
  //    (mainly cabinet) by deconvolution

  // Get test sweep signal
  QVector<float> sweepSignal(processor->getSamplingRate() * 10);
  generate_logarithmic_sweep(10.0, processor->getSamplingRate(),
                             20.0, responseDataSamplerate / 2.0,
                             0.01,
                             sweepSignal.data());

  // Get test signal after preamp - convolve test sweep
  // with preamp impulse response
  fft_convolver(sweepSignal.data(), sweepSignal.size(), preamp_impulse.data(),
                preamp_impulse.size());

  // Get sweep response from profiled amplifier
  QVector<float> sweepResponseL;
  QVector<float> sweepResponseR;

  int sweepStart = ((TEST_SIGNAL_LENGTH_SEC + TEST_SIGNAL_PAUSE_LENGTH_SEC) *
    EXPER_POINTS_NUM + 1) * responseDataSamplerate * responseDataChannels;

  for (int i = 0;
       i < (responseDataSamplerate * 11 * responseDataChannels);
       i += responseDataChannels)
  {
    sweepResponseL.append(responseData[sweepStart + i]);
    float sum = 0.0;
    for (int j = 1; j < responseDataChannels; j++)
    {
      sum += responseData[sweepStart + i + j];
    }
    sum /= (responseDataChannels - 1);
    sweepResponseR.append(sum);
  }

  // Resample to processor sampling rate
  QVector<float> sweepResponseResampledL = resample_vector(
    sweepResponseL,
    responseDataSamplerate,
    processor->getSamplingRate()
  );

  QVector<float> sweepResponseResampledR = resample_vector(
    sweepResponseR,
    responseDataSamplerate,
    processor->getSamplingRate()
  );

  // Calculate cabinet impulse response by deconvolution
  // with test signal after preamp
  QVector<float> cabinet_impulseL(processor->getSamplingRate());
  QVector<float> cabinet_impulseR(processor->getSamplingRate());

  fft_deconvolver(sweepSignal.data(),
                     sweepSignal.size(),
                     sweepResponseResampledL.data(),
                     sweepResponseResampledL.size(),
                     cabinet_impulseL.data(),
                     cabinet_impulseL.size(),
                     30.0 / processor->getSamplingRate(),
                     10000.0 / processor->getSamplingRate()
                 );

  fft_deconvolver(sweepSignal.data(),
                     sweepSignal.size(),
                     sweepResponseResampledR.data(),
                     sweepResponseResampledR.size(),
                     cabinet_impulseR.data(),
                     cabinet_impulseR.size(),
                     30.0 / processor->getSamplingRate(),
                     10000.0 / processor->getSamplingRate()
                 );

  emit progressChanged(75);

  // 4. Correct cabinet impulse response
  //    by auto-equalization based on the "real" test.
  //    Real test signal is recorded DI from guitar.

  // Get response on real test signal from profiled amplifier
  QVector<float> realTestResponseL;
  QVector<float> realTestResponseR;

  int realTestStart = ((TEST_SIGNAL_LENGTH_SEC + TEST_SIGNAL_PAUSE_LENGTH_SEC) *
    EXPER_POINTS_NUM + 1) * responseDataSamplerate * responseDataChannels +
    responseDataSamplerate * responseDataChannels * 11;

  for (int i = 0;
         i < (responseData.size() - realTestStart);
         i += responseDataChannels)
  {
    realTestResponseL.append(responseData[realTestStart + i]);
    float sum = 0.0;
    for (int j = 1; j < responseDataChannels; j++)
    {
      sum += responseData[realTestStart + i + j];
    }
    sum /= (responseDataChannels - 1.0);
    realTestResponseR.append(sum);
  }

  QVector<float> realTestResponseResampledL = resample_vector(realTestResponseL,
    responseDataSamplerate, processor->getSamplingRate());

  QVector<float> realTestResponseResampledR = resample_vector(realTestResponseR,
    responseDataSamplerate, processor->getSamplingRate());

  QVector<float> realTestSignal;
  realTestSignal = loadRealTestFile(realTestSignal, processor->getSamplingRate());

  float realTestSignalRMS = 0.0;
  for (int i = 0; i<realTestSignal.size(); i++)
  {
    realTestSignalRMS += pow(realTestSignal[i], 2);
  }
  realTestSignalRMS = sqrt(realTestSignalRMS / realTestSignal.size());

  // Normalize to -20 dB standart input level
  for (int i = 0; i<realTestSignal.size(); i++)
  {
    realTestSignal[i] *= 0.1 / realTestSignalRMS;
  }

  // Normalize preamp and cabinet impulses to standard level (my standard :))
  float max_val = 0.0;
  for (int i = 0; i < preamp_impulse.size(); i++)
  {
    if (fabs(preamp_impulse[i]) > max_val)
    {
      max_val = fabs(preamp_impulse[i]);
    }
  }

  max_val /= 0.4 * (48000.0 / (float)processor->getSamplingRate());

  for (int i = 0; i < preamp_impulse.size(); i++)
  {
    preamp_impulse[i] /= max_val;
  }

  float cabinetImpulseEnergy = 0.0;

  for (int i = 0; i < cabinet_impulseL.size(); i++)
  {
    cabinetImpulseEnergy += pow((cabinet_impulseL[i] + cabinet_impulseR[i]) / 2.0, 2);
  }

  float cabinetImpulseEnergyCoeff = sqrt(0.45 * 48000.0 /
    (float)processor->getSamplingRate()) /
    sqrt(cabinetImpulseEnergy);

  for (int i = 0; i < cabinet_impulseL.size(); i++)
  {
    cabinet_impulseL[i] *= cabinetImpulseEnergyCoeff;
    cabinet_impulseR[i] *= cabinetImpulseEnergyCoeff;
  }

  QVector<float> processedDataL(realTestSignal.size());
  QVector<float> processedDataR(realTestSignal.size());

  // Create background Processor with previously adjusted profile
  QSharedPointer<Processor> backProcessor
    = QSharedPointer<Processor>(new Processor(processor->getSamplingRate()));
  backProcessor->loadProfile(processor->getProfileFileName());

  stControls ctrls = processor->getControls();
  ctrls.drive = 100.0;
  ctrls.mastergain = 100.0;
  backProcessor->setControls(ctrls);
  processor->setControls(ctrls);

  st_profile profile = processor->getProfile();

  if (preset == CRYSTALCLEAN_PRESET)
  {
    profile.preamp_level = 0.005;
    profile.amp_level = 0.1;

    profile.signature[0] = 'T';
    profile.signature[1] = 'a';
    profile.signature[2] = 'P';
    profile.signature[3] = 'f';

    profile.version = 1;

    profile.preamp_bias = 0.0;
    profile.preamp_Kreg = 0.8;
    profile.preamp_Upor = 0.8;

    profile.tonestack_low_freq = 20.0;
    profile.tonestack_low_band = 400.0;
    profile.tonestack_middle_freq = 500.0;
    profile.tonestack_middle_band = 400.0;
    profile.tonestack_high_freq = 10000.0;
    profile.tonestack_high_band = 18000.0;

    profile.amp_bias = 0.0;
    profile.amp_Kreg = 0.39;
    profile.amp_Upor = 0.91;

    profile.sag_time = 0.3;
    profile.sag_coeff = 0.0;

    profile.output_level = 0.34;
  }

  if (preset == CLASSIC_PRESET)
  {
    if (desiredGain <= 0.05)
    {
      profile.preamp_level = 0.005;
      profile.amp_level = desiredGain / 0.005;
    }
    else
    {
      profile.amp_level = 10.0;
      profile.preamp_level = desiredGain / 10.0;
    }

    profile.signature[0] = 'T';
    profile.signature[1] = 'a';
    profile.signature[2] = 'P';
    profile.signature[3] = 'f';

    profile.version = 1;

    profile.preamp_bias = 0.0;
    profile.preamp_Kreg = 0.8;
    profile.preamp_Upor = 0.8;

    profile.tonestack_low_freq = 20.0;
    profile.tonestack_low_band = 400.0;
    profile.tonestack_middle_freq = 500.0;
    profile.tonestack_middle_band = 400.0;
    profile.tonestack_high_freq = 10000.0;
    profile.tonestack_high_band = 18000.0;

    profile.amp_bias = 0.2;
    profile.amp_Kreg = 0.7;
    profile.amp_Upor = 0.2;

    profile.sag_time = 0.3;
    profile.sag_coeff = 0.5;

    profile.output_level = 1.0/7.5;
  }

  if (preset == MASTERGAIN_PRESET)
  {
    if (desiredGain > 3.16)
    {
      desiredGain = 3.16;
    }

    profile.preamp_level = desiredGain / 0.1;
    profile.amp_level = 0.1;

    profile.signature[0] = 'T';
    profile.signature[1] = 'a';
    profile.signature[2] = 'P';
    profile.signature[3] = 'f';

    profile.version = 1;

    profile.preamp_bias = 0.0; //!!!!!!!!!!!!!!!!!!!!!!
    profile.preamp_Kreg = 2.0;
    profile.preamp_Upor = 0.2;

    profile.tonestack_low_freq = 20.0;
    profile.tonestack_low_band = 400.0;
    profile.tonestack_middle_freq = 500.0;
    profile.tonestack_middle_band = 400.0;
    profile.tonestack_high_freq = 10000.0;
    profile.tonestack_high_band = 18000.0;

    profile.amp_bias = 0.2;
    profile.amp_Kreg = 1.0;
    profile.amp_Upor = 0.5;

    profile.sag_time = 0.1;
    profile.sag_coeff = 0.0;

    profile.output_level = 1/5.0;
  }

  backProcessor->setProfile(profile);

  // Set adjusted profile to the main Processor
  processor->setProfile(profile);

  backProcessor->setPreampImpulse(preamp_impulse);
  backProcessor->setCabinetImpulse(cabinet_impulseL, cabinet_impulseR);

  // Cut signals up to multiple of FRAGM (64 samples)
  int sizeToFragm = floor(realTestSignal.size() / (double)fragm) * fragm;

  realTestSignal.resize(sizeToFragm);
  processedDataL.resize(sizeToFragm);
  processedDataR.resize(sizeToFragm);
  realTestResponseResampledL.resize(sizeToFragm);
  realTestResponseResampledR.resize(sizeToFragm);

  // Get real test response from previously adjusted Processor
  backProcessor->process(processedDataL.data(),
                         processedDataR.data(),
                         realTestSignal.data(),
                         realTestSignal.size());

  emit progressChanged(85);

  QVector<double> processedDataDouble(processedDataL.size());

  for (int i = 0; i < processedDataL.size(); i++)
  {
    processedDataDouble[i] = (processedDataL[i] + processedDataR[i]) / 2.0;
  }

  QVector<double> realTestResponseResampledDouble(realTestResponseResampledL.size());

  for (int i = 0; i < realTestResponseResampledL.size(); i++)
  {
    realTestResponseResampledDouble[i] = (realTestResponseResampledL[i] +
      realTestResponseResampledR[i]) / 2.0;
  }

  // Calculate auto-equalizer correction
  // between real test responses from Processor
  // and from profiled amplifier
  int averageSpectrumSize = 4096;
  int autoEqualazierPointsNum = 40;

  processor->correctionEqualizerFLogValues.resize(autoEqualazierPointsNum);
  processor->correctionEqualizerDbValues.resize(autoEqualazierPointsNum);

  processor->correctionEqualizerFLogValues[0] = log10(10.0);

  for (int i = 0; i < autoEqualazierPointsNum - 1; i++)
  {
    processor->correctionEqualizerFLogValues[i + 1] = (log10(20000.0) - log10(10.0)) *
      (double)(i + 1) / (processor->correctionEqualizerFLogValues.size() - 1) + log10(10.0);
  }

  calulate_autoeq_amplitude_response(averageSpectrumSize,
                                     backProcessor->getSamplingRate(),
                                     processedDataDouble.data(),
                                     processedDataDouble.size(),
                                     realTestResponseResampledDouble.data(),
                                     realTestResponseResampledDouble.size(),
                                     processor->correctionEqualizerFLogValues.data(),
                                     processor->correctionEqualizerDbValues.data(),
                                     autoEqualazierPointsNum
                                     );

  for (int i = 0; i < processor->correctionEqualizerFLogValues.size(); i++)
  {
    if (processor->correctionEqualizerDbValues[i] > 20.0)
    {
      processor->correctionEqualizerDbValues[i] = 20.0;
    }

    if (processor->correctionEqualizerDbValues[i] < (-30.0))
    {
      processor->correctionEqualizerDbValues[i] = -30.0;
    }
  }

  processor->setPreampImpulse(preamp_impulse);
  processor->setCabinetImpulse(cabinet_impulseL, cabinet_impulseR);

  QVector<double> w(processor->correctionEqualizerFLogValues.size());
  QVector<double> A(processor->correctionEqualizerFLogValues.size());

  for (int i = 0; i < w.size(); i++)
  {
    w[i] = 2.0 * M_PI * pow(10.0, processor->correctionEqualizerFLogValues[i]);
    A[i] = pow(10.0, processor->correctionEqualizerDbValues[i] / 20.0);
  }

  // Set correction frequency response to the main Processor
  processor->setCabinetSumCorrectionImpulseFromFrequencyResponse(w, A);

  // Process dummy data to apply changes
  QVector<float> dummyData(fragm);
  processor->process(dummyData.data(),
                     dummyData.data(),
                     dummyData.data(),
                     fragm);

  // Apply cabinet frequency response correction
  // to cabinet impulse response
  processor->applyCabinetSumCorrection();
  processor->resetCabinetSumCorrection();

  processor->correctionEqualizerFLogValues.resize(4);
  processor->correctionEqualizerDbValues.resize(4);

  processor->correctionEqualizerFLogValues[0] = log10(10.0);
  processor->correctionEqualizerFLogValues[1] = log10(1000.0);
  processor->correctionEqualizerFLogValues[2] = log10(20000.0);
  processor->correctionEqualizerFLogValues[3] = log10(22000.0);

  processor->correctionEqualizerDbValues[0] = 0.0;
  processor->correctionEqualizerDbValues[1] = 0.0;
  processor->correctionEqualizerDbValues[2] = 0.0;
  processor->correctionEqualizerDbValues[3] = 0.0;

  // Process dummy data to apply changes
  processor->process(dummyData.data(), dummyData.data(), dummyData.data(), fragm);

  processor->setProfileFileName(":/profiles/British Crunch.tapf");

  // Send real test DI sound and real test response
  // from profiled amplifier to the Player
  player->setDiData(realTestSignal);
  player->setRefData(realTestResponseResampledL, realTestResponseResampledR);

  // Equalize RMS of the real test response from profiled amplifier
  // with sound from the Processor
  player->equalDataRMS();
}

void Profiler::createTestFile(QString fileName, int version)
{
  switch (version)
  {
    case 0:
      createTestFile_v1(fileName);
    break;
  }
}

void Profiler::createTestFile_v1(QString fileName)
{
  // Define some variables for the sound
  float sampleRate = 44100.0; // hertz

  int nSamples_signal = (int)(TEST_SIGNAL_LENGTH_SEC * sampleRate);
  int nSamples_pause = (int)(TEST_SIGNAL_PAUSE_LENGTH_SEC * sampleRate);

  QVector<float> testSignal;

  // Create 1 sec of silence
  for (int i = 0; i < sampleRate * 1; i++)
  {
    testSignal.append(0.0);
  }

  // Create "crunch" test signal
  for (int j = 0; j < EXPER_POINTS_NUM; j++)
  {
    int i;
    float frameData;
    for(i = 0; i < nSamples_signal; i++ )
    {
      frameData = freq_weights[j] * pow((double)i / ((double)nSamples_signal -
        1.0), 2) * sin(calibration_freqs[j] * (double)i / sampleRate);

      testSignal.append(frameData);
    }

    for (int k = testSignal.size() - 1; k >= 0; k--)
    {
      if ((testSignal[k] * testSignal[k - 1]) < 0.0)
      {
        break;
      }
      else
      {
        testSignal[k] = 0.0;
      }
    }

    for(i=0; i < nSamples_pause; i++ )
    {
      frameData = 0;
      testSignal.append(frameData);
    }
  }

  // 1 sec of silence
  for (int i = 0; i < sampleRate * 1; i++)
  {
    testSignal.append(0.0);
  }

  // Create sweep signal
  int startSweep = testSignal.length();
  testSignal.resize(testSignal.length() + sampleRate * 10);

  generate_logarithmic_sweep(10.0, sampleRate, 20.0, sampleRate / 2.0,
                             0.01, &(testSignal.data()[startSweep]));

  int startBlank = testSignal.size();
  testSignal.resize(testSignal.size() + sampleRate * 1);

  for (int i = 0; i < sampleRate * 1; i++)
  {
    testSignal[i + startBlank] = 0.0;
  }

  // Add "real" test signal from wav file
  testSignal = loadRealTestFile(testSignal, sampleRate);

  SF_INFO sfinfo;
  sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
  sfinfo.frames = testSignal.size();
  sfinfo.samplerate = (int)sampleRate;
  sfinfo.channels = 1;
  sfinfo.sections = 1;
  sfinfo.seekable = 1;

  SNDFILE *testFile = sf_open(fileName.toUtf8().constData(), SFM_WRITE, &sfinfo);

  if (testFile != NULL)
  {
    sf_writef_float(testFile, testSignal.data(), testSignal.size());
  }

  sf_close(testFile);
}

void ProfilerThread::run()
{
  profiler->analyze(presetType);
}

void Profiler::warningMessageNeededSlot(QString message)
{
  QMessageBox::warning(nullptr, QObject::tr("Warning!"),
                               message,
                               QMessageBox::Ok,
                               QMessageBox::Ok);
}
