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

#include <sndfile.h>
#include <cmath>
#include <QSharedPointer>

#include "player.h"

#define SAMPLE_RATE         (48000)
#define PA_SAMPLE_TYPE      paFloat32
#define FRAMES_PER_BUFFER   (256)

#define RMS_COUNT_MAX 4800

int peakRMScount = 0;
double peakInputRMSsum = 0.0;
double peakOutputRMSsum = 0.0;

static int process( const void *inputBuffer, void *outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData )
{
  float *out = (float*)outputBuffer;
  float *in = (float*)inputBuffer;
  (void) timeInfo; /* Prevent unused variable warnings. */
  (void) statusFlags;
  Player *inst = (Player *)userData;

  QVector<float> outL(framesPerBuffer);
  QVector<float> outR(framesPerBuffer);

  switch (inst->status)
  {
    case Player::PlayerStatus::PS_STOP:
    {
      inst->diPos = 0;
      inst->refPos = 0;

      memset(outL.data(), 0, sizeof (float) * framesPerBuffer);
      memset(outR.data(), 0, sizeof (float) * framesPerBuffer);
    }
    break;
    case Player::PlayerStatus::PS_PAUSE:
    {
      memset(outL.data(), 0, sizeof (float) * framesPerBuffer);
      memset(outR.data(), 0, sizeof (float) * framesPerBuffer);
    }
    break;
    case Player::PlayerStatus::PS_PLAY_DI:
    {
      if (inst->diData.size() != 0)
      {
        if ((inst->diPos + framesPerBuffer) > (unsigned int)inst->diData.size())
        {
          QVector<float> tempBuffer(framesPerBuffer);

          for (unsigned int i = inst->diPos;
               i < (unsigned int)inst->diData.size(); i++)
               {
                 tempBuffer[i - inst->diPos] = inst->diData[i];
                 peakInputRMSsum += pow(inst->diData[i], 2);
               }

               for (unsigned int i = 0;
                    i < (framesPerBuffer - inst->diData.size() + inst->diPos); i++)
                    {
                      tempBuffer[i + inst->diData.size() - inst->diPos] = inst->diData[i];
                      peakInputRMSsum += pow(inst->diData[i], 2);
                      inst->incRMScounter();
                    }

                    inst->processor->process(outL.data(),
                                             outR.data(),
                                             tempBuffer.data(),
                                             framesPerBuffer);

                    for (unsigned int i = 0; i < framesPerBuffer; i++)
                    {
                      peakOutputRMSsum += pow(outL[i], 2);
                    }

                    inst->diPos = inst->diPos + framesPerBuffer - inst->diData.size();
        }
        else
        {
          for (unsigned int i = 0; i < framesPerBuffer; i++)
          {
            peakInputRMSsum += pow(inst->diData[i + inst->diPos], 2);
            inst->incRMScounter();
          }
          inst->processor->process(outL.data(),
                                   outR.data(),
                                   inst->diData.data() + inst->diPos,
                                   framesPerBuffer);

          for (unsigned int i = 0; i < framesPerBuffer; i++)
          {
            peakOutputRMSsum += pow(outL[i], 2);
          }

          inst->diPos += framesPerBuffer;
        }

        if ((inst->refPos + framesPerBuffer) > (unsigned int)inst->refDataL.size())
        {
          inst->refPos = inst->refPos + framesPerBuffer - inst->refDataL.size();
        }
        else
        {
          inst->refPos += framesPerBuffer;
        }
      }
      else
      {
        memset(outL.data(), 0, sizeof (float) * framesPerBuffer);
        memset(outR.data(), 0, sizeof (float) * framesPerBuffer);
      }
    }
    break;
    case Player::PlayerStatus::PS_PLAY_REF:
    {
      if (inst->refDataL.size() != 0)
      {
        if ((inst->refPos + framesPerBuffer) > (unsigned int)inst->refDataL.size())
        {
          for (int i = inst->refPos; i < inst->refDataL.size(); i++)
          {
            outL[i - inst->refPos] = inst->refDataL[i] * inst->getLevel();
            peakInputRMSsum += 0.0;
            peakOutputRMSsum += pow(outL[i - inst->refPos], 2);
            inst->incRMScounter();
          }

          for (unsigned int i = 0;
               i < (framesPerBuffer - inst->refDataL.size() + inst->refPos); i++)
               {
                 outL[i + inst->refDataL.size() - inst->refPos] = inst->refDataL[i] *
                 inst->getLevel();
                 peakInputRMSsum += 0.0;
                 peakOutputRMSsum += pow(outL[i + inst->refDataL.size() - inst->refPos], 2);
                 inst->incRMScounter();
               }

               for (int i = inst->refPos; i < inst->refDataR.size(); i++)
               {
                 outR[i - inst->refPos] = inst->refDataR[i] * inst->getLevel();
               }

               for (unsigned int i = 0;
                    i < (framesPerBuffer - inst->refDataR.size() + inst->refPos); i++)
                    {
                      outR[i + inst->refDataR.size() - inst->refPos] = inst->refDataR[i] *
                      inst->getLevel();
                    }

                    inst->refPos = inst->refPos + framesPerBuffer - inst->refDataL.size();
        }
        else
        {
          for (unsigned int i = inst->refPos; i < framesPerBuffer + inst->refPos; i++)
          {
            outL[i - inst->refPos] = inst->refDataL[i] * inst->getLevel();
            peakInputRMSsum += 0.0;
            peakOutputRMSsum += pow(outL[i - inst->refPos], 2);
            inst->incRMScounter();
          }

          for (unsigned int i = inst->refPos; i < framesPerBuffer + inst->refPos; i++)
          {
            outR[i - inst->refPos] = inst->refDataR[i] * inst->getLevel();
          }

          inst->refPos += framesPerBuffer;
        }

        if ((inst->diPos + framesPerBuffer) > (unsigned int)inst->diData.size())
        {
          inst->diPos = inst->diPos + framesPerBuffer - inst->diData.size();
        }
        else
        {
          inst->diPos += framesPerBuffer;
        }
      }
      else
      {
        memset(outL.data(), 0, sizeof (float) * framesPerBuffer);
        memset(outR.data(), 0, sizeof (float) * framesPerBuffer);
      }
    }
    break;
    case Player::PlayerStatus::PS_MONITOR:
    {
      for (unsigned int i = 0; i < framesPerBuffer; i++)
      {
        in[i] *= inst->inputLevel;
        peakInputRMSsum += pow(in[i], 2);
        inst->incRMScounter();
      }
      inst->processor->process(outL.data(), outR.data(), in, framesPerBuffer);

      for (unsigned int i = 0; i < framesPerBuffer; i++)
      {
        peakOutputRMSsum += pow(outL[i], 2);
      }
    }
    break;
  }

  for (int i = 0; i < outL.size(); i++)
  {
    out[i*2] = outL[i];
    out[i*2 + 1] = outR[i];
  }
  return paContinue;
}


/*static int process (jack_nframes_t nframes, void *arg)
{
  jack_default_audio_sample_t *in, *outL, *outR;

  Player *inst = (Player *)arg;

  in = (jack_default_audio_sample_t *)jack_port_get_buffer(inst->input_port,
                                                           nframes);
  outL = (jack_default_audio_sample_t *)jack_port_get_buffer (inst->output_port_left,
                                                              nframes);
  outR = (jack_default_audio_sample_t *)jack_port_get_buffer (inst->output_port_right,
                                                              nframes);

  switch (inst->status)
  {
    case Player::PlayerStatus::PS_STOP:
    {
      inst->diPos = 0;
      inst->refPos = 0;

      memset(outL, 0, sizeof (jack_default_audio_sample_t) * nframes);
      memset(outR, 0, sizeof (jack_default_audio_sample_t) * nframes);
    }
    break;
    case Player::PlayerStatus::PS_PAUSE:
    {
      memset(outL, 0, sizeof (jack_default_audio_sample_t) * nframes);
      memset(outR, 0, sizeof (jack_default_audio_sample_t) * nframes);
    }
    break;
    case Player::PlayerStatus::PS_PLAY_DI:
    {
      if (inst->diData.size() != 0)
      {
        if ((inst->diPos + nframes) > (unsigned int)inst->diData.size())
        {
          QVector<float> tempBuffer(nframes);

          for (unsigned int i = inst->diPos;
               i < (unsigned int)inst->diData.size(); i++)
          {
            tempBuffer[i - inst->diPos] = inst->diData[i];
            peakInputRMSsum += pow(inst->diData[i], 2);
          }

          for (unsigned int i = 0;
               i < (nframes - inst->diData.size() + inst->diPos); i++)
          {
            tempBuffer[i + inst->diData.size() - inst->diPos] = inst->diData[i];
            peakInputRMSsum += pow(inst->diData[i], 2);
            inst->incRMScounter();
          }

          inst->processor->process(outL,
                                   outR,
                                   tempBuffer.data(),
                                   nframes);

          for (unsigned int i = 0; i < nframes; i++)
          {
            peakOutputRMSsum += pow(outL[i], 2);
          }

          inst->diPos = inst->diPos + nframes - inst->diData.size();
        }
        else
        {
          for (unsigned int i = 0; i < nframes; i++)
          {
            peakInputRMSsum += pow(inst->diData[i + inst->diPos], 2);
            inst->incRMScounter();
          }
          inst->processor->process(outL,
                                   outR,
                                   inst->diData.data() + inst->diPos,
                                   nframes);

          for (unsigned int i = 0; i < nframes; i++)
          {
            peakOutputRMSsum += pow(outL[i], 2);
          }

          inst->diPos += nframes;
        }

        if ((inst->refPos + nframes) > (unsigned int)inst->refDataL.size())
        {
          inst->refPos = inst->refPos + nframes - inst->refDataL.size();
        }
        else
        {
          inst->refPos += nframes;
        }
      }
      else
      {
        memset(outL, 0, sizeof (jack_default_audio_sample_t) * nframes);
        memset(outR, 0, sizeof (jack_default_audio_sample_t) * nframes);
      }
    }
    break;
    case Player::PlayerStatus::PS_PLAY_REF:
    {
      if (inst->refDataL.size() != 0)
      {
        if ((inst->refPos + nframes) > (unsigned int)inst->refDataL.size())
        {
          for (int i = inst->refPos; i < inst->refDataL.size(); i++)
          {
            outL[i - inst->refPos] = inst->refDataL[i] * inst->getLevel();
            peakInputRMSsum += 0.0;
            peakOutputRMSsum += pow(outL[i - inst->refPos], 2);
            inst->incRMScounter();
          }

          for (unsigned int i = 0;
               i < (nframes - inst->refDataL.size() + inst->refPos); i++)
          {
            outL[i + inst->refDataL.size() - inst->refPos] = inst->refDataL[i] *
              inst->getLevel();
            peakInputRMSsum += 0.0;
            peakOutputRMSsum += pow(outL[i + inst->refDataL.size() - inst->refPos], 2);
            inst->incRMScounter();
          }

          for (int i = inst->refPos; i < inst->refDataR.size(); i++)
          {
            outR[i - inst->refPos] = inst->refDataR[i] * inst->getLevel();
          }

          for (unsigned int i = 0;
               i < (nframes - inst->refDataR.size() + inst->refPos); i++)
          {
            outR[i + inst->refDataR.size() - inst->refPos] = inst->refDataR[i] *
              inst->getLevel();
          }

          inst->refPos = inst->refPos + nframes - inst->refDataL.size();
        }
        else
        {
          for (unsigned int i = inst->refPos; i < nframes + inst->refPos; i++)
          {
            outL[i - inst->refPos] = inst->refDataL[i] * inst->getLevel();
            peakInputRMSsum += 0.0;
            peakOutputRMSsum += pow(outL[i - inst->refPos], 2);
            inst->incRMScounter();
          }

          for (unsigned int i = inst->refPos; i < nframes + inst->refPos; i++)
          {
            outR[i - inst->refPos] = inst->refDataR[i] * inst->getLevel();
          }

          inst->refPos += nframes;
        }

        if ((inst->diPos + nframes) > (unsigned int)inst->diData.size())
        {
          inst->diPos = inst->diPos + nframes - inst->diData.size();
        }
        else
        {
          inst->diPos += nframes;
        }
      }
      else
      {
        memset(outL, 0, sizeof (jack_default_audio_sample_t) * nframes);
        memset(outR, 0, sizeof (jack_default_audio_sample_t) * nframes);
      }
    }
    break;
    case Player::PlayerStatus::PS_MONITOR:
    {
      for (unsigned int i = 0; i < nframes; i++)
      {
        in[i] *= inst->inputLevel;
        peakInputRMSsum += pow(in[i], 2);
        inst->incRMScounter();
      }
      inst->processor->process(outL, outR, in, nframes);

      for (unsigned int i = 0; i < nframes; i++)
      {
        peakOutputRMSsum += pow(outL[i], 2);
      }
    }
    break;
  }
  return 0;
}*/

Player::Player()
{
  simple_quit = 0;
  diPos = 0;
  refPos = 0;

  status = PS_STOP;

  equalDataRMSThread = new EqualDataRMSThread();
  connect(equalDataRMSThread, &QThread::finished, this,
          &Player::equalDataRMSThreadFinished);
}

Player::~Player()
{
  Pa_Terminate();
}

void Player::incRMScounter()
{
  if (peakRMScount < RMS_COUNT_MAX)
  {
    peakRMScount++;
  }
  else
  {
    float peakInputRMSvalue = sqrt(peakInputRMSsum / peakRMScount);
    float peakOutputRMSvalue = sqrt(peakOutputRMSsum / peakRMScount);

    peakRMScount = 0;
    peakInputRMSsum = 0.0;
    peakOutputRMSsum = 0.0;

    emit peakRMSValueCalculated(peakInputRMSvalue, peakOutputRMSvalue);
  }
}

int Player::connectToPortAudio()
{
  err = Pa_Initialize();
  if( err != paNoError ) return 1;

  inputParameters.device = Pa_GetDefaultInputDevice(); /* default input device */
  if (inputParameters.device == paNoDevice) {
    fprintf(stderr,"Error: No default input device.\n");
    return 1;
  }
  inputParameters.channelCount = 1;       /* stereo input */
  inputParameters.sampleFormat = PA_SAMPLE_TYPE;
  inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
  inputParameters.hostApiSpecificStreamInfo = NULL;

  outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
  if (outputParameters.device == paNoDevice) {
    fprintf(stderr,"Error: No default output device.\n");
    return 1;
  }
  outputParameters.channelCount = 2;       /* stereo output */
  outputParameters.sampleFormat = PA_SAMPLE_TYPE;
  outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
  outputParameters.hostApiSpecificStreamInfo = NULL;

  err = Pa_OpenStream(
    &stream,
    &inputParameters,
    &outputParameters,
    SAMPLE_RATE,
    FRAMES_PER_BUFFER,
    0, /* paClipOff, */  /* we won't output out of range samples so don't bother clipping them */
    process,
    this );
  if( err != paNoError ) return 1;

  sampleRate = SAMPLE_RATE;

  return 0;
}

int Player::activate()
{
  err = Pa_StartStream( stream );
  if( err != paNoError ) return 1;

  return 0;
}

void Player::setDiData(QVector<float> data)
{
  diData = data;
  emit dataChanged();
}

void Player::setRefData(QVector<float> dataL, QVector<float> dataR)
{
  refDataL = dataL;
  refDataR = dataR;
  emit dataChanged();
}

void Player::setStatus(PlayerStatus newStatus)
{
  status = newStatus;
}

void Player::setProcessor(Processor *prc)
{
  processor = prc;
}

int Player::getSampleRate()
{
  return sampleRate;
}

void Player::equalDataRMS()
{
  if (isEqualDataRMSThreadRunning)
  {
    return;
  }

  if (refDataL.size() == 0)
  {
    emit equalRMSFinished();
    return;
  }

  equalDataRMSThread->processor = processor;
  equalDataRMSThread->player = this;
  equalDataRMSThread->start();
  isEqualDataRMSThreadRunning = true;
}

void Player::equalDataRMSThreadFinished()
{
  isEqualDataRMSThreadRunning = false;
  emit equalRMSFinished();
}

void Player::setLevel(float lev)
{
  level = lev;
}

float Player::getLevel()
{
  return level;
}

void Player::setInputLevel(float dbInputLevel)
{
  inputLevel = pow(10.0, dbInputLevel / 20.0);
}

void EqualDataRMSThread::run()
{
  QVector<float> processedDataL(player->diData.size());
  QVector<float> processedDataR(player->diData.size());

  QSharedPointer<Processor> backProcessor
    = QSharedPointer<Processor>(new Processor(processor->getSamplingRate()));

  backProcessor->loadProfile(processor->getProfileFileName());

  backProcessor->setControls(processor->getControls());
  backProcessor->setProfile(processor->getProfile());

  backProcessor->setPreampImpulse(processor->getPreampImpulse());
  backProcessor->setCabinetImpulse(processor->getLeftImpulse(),
                                   processor->getRightImpulse());

  QVector<double> w(processor->correctionEqualizerFLogValues.size());
  QVector<double> A(processor->correctionEqualizerFLogValues.size());

  if (processor->correctionEqualizerFLogValues.size() >= 3)
  {
    for (int i = 0; i < w.size(); i++)
    {
      w[i] = 2.0 * M_PI * pow(10.0, processor->correctionEqualizerFLogValues[i]);
      A[i] = pow(10.0, processor->correctionEqualizerDbValues[i] / 20.0);
    }

    backProcessor->setCabinetSumCorrectionImpulseFromFrequencyResponse(w, A);
  }

  if (processor->preampCorrectionEqualizerFLogValues.size() >= 3)
  {
    w.resize(processor->preampCorrectionEqualizerFLogValues.size());
    A.resize(processor->preampCorrectionEqualizerFLogValues.size());

    for (int i = 0; i < w.size(); i++)
    {
      w[i] = 2.0 * M_PI * pow(10.0, processor->preampCorrectionEqualizerFLogValues[i]);
      A[i] = pow(10.0, processor->preampCorrectionEqualizerDbValues[i] / 20.0);
    }

    backProcessor->setPreampCorrectionImpulseFromFrequencyResponse(w, A);
  }

  int sizeToFragm = floor(player->diData.size() / (double)fragm) * fragm;

  processedDataL.resize(sizeToFragm);
  processedDataR.resize(sizeToFragm);

  backProcessor->process(processedDataL.data(),
                         processedDataR.data(),
                         player->diData.data(),
                         sizeToFragm);

  double rmsProcessedData = 0.0;
  for (int i = 0; i < processedDataL.size(); i++)
  {
    rmsProcessedData += pow((processedDataL[i] +
      processedDataR[i]) / 2.0, 2);
  }
  rmsProcessedData = sqrt(rmsProcessedData / processedDataL.size());

  double rmsRefData = 0.0;
  for (int i = 0; i < player->refDataL.size(); i++)
  {
    rmsRefData += pow((player->refDataL[i] + player->refDataR[i]) / 2.0, 2);
  }
  rmsRefData = sqrt(rmsRefData / player->refDataL.size());

  double rmsRatio = rmsRefData / rmsProcessedData;

  for (int i = 0; i < player->refDataL.size(); i++)
  {
    player->refDataL[i] /= rmsRatio;
    player->refDataR[i] /= rmsRatio;
  }
}
