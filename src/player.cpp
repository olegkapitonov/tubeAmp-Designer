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

#define RMS_COUNT_MAX 4800

int peakRMScount = 0;
double peakInputRMSsum = 0.0;
double peakOutputRMSsum = 0.0;

static int process (jack_nframes_t nframes, void *arg)
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
}

static void session_callback(jack_session_event_t *event, void *arg)
{
  Player *inst = (Player *)arg;

  char retval[100];
  printf ("session notification\n");
  printf ("path %s, uuid %s, type: %s\n", event->session_dir,
          event->client_uuid, event->type == JackSessionSave ? "save" : "quit");


  snprintf (retval, 100, "jack_simple_session_client %s", event->client_uuid);
  event->command_line = strdup (retval);

  jack_session_reply( inst->client, event );

  if (event->type == JackSessionSaveAndQuit) {
    inst->simple_quit = 1;
  }

  jack_session_event_free(event);
}

static void jack_shutdown(void*)
{
	exit (1);
}

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
  jack_client_close(client);
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

int Player::connectToJack()
{
  jack_status_t status;
  const char *client_name = "tubeAmp Designer";

  /* open a client connection to the JACK server */

  client = jack_client_open (client_name, JackNoStartServer, &status );

  if (client == NULL)
  {
    fprintf (stderr, "jack_client_open() failed, status = 0x%2.0x\n", status);
    if (status & JackServerFailed)
    {
      fprintf (stderr, "Unable to connect to JACK server\n");
    }
    return 1;
  }
  if (status & JackServerStarted)
  {
    fprintf (stderr, "JACK server started\n");
  }
  if (status & JackNameNotUnique)
  {
    client_name = jack_get_client_name(client);
  }

  /* tell the JACK server to call `process()' whenever
    there is work to be done.
  */

  jack_set_process_callback(client, process, this);

  /* tell the JACK server to call `jack_shutdown()' if
  it ever shuts down, either entirely, or if it
  just decides to stop calling us.
  */

  jack_on_shutdown(client, jack_shutdown, this);

  /* tell the JACK server to call `session_callback()' if
  the session is saved.
  */

  jack_set_session_callback(client, session_callback, this);

  /* display the current sample rate.
  */

  printf ("engine sample rate: %" PRIu32 "\n", jack_get_sample_rate (client));
  sampleRate = jack_get_sample_rate (client);

/* create two ports */

  input_port = jack_port_register (client, "input",
    JACK_DEFAULT_AUDIO_TYPE,
    JackPortIsInput, 0);

  output_port_left = jack_port_register (client, "outputL",
  JACK_DEFAULT_AUDIO_TYPE,
    JackPortIsOutput, 0);

  output_port_right = jack_port_register (client, "outputR",
  JACK_DEFAULT_AUDIO_TYPE,
    JackPortIsOutput, 0);

  if ((input_port == NULL) || (output_port_left == NULL) || (output_port_right == NULL))
  {
    fprintf(stderr, "no more JACK ports available\n");
    return 1;
  }

  return 0;
}

int Player::activate()
{
  if (jack_activate(client))
  {
    fprintf (stderr, "cannot activate client");
    return 1;
  }

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
