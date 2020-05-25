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

#include <QPalette>
#include <QFrame>
#include <QVBoxLayout>
#include <QWidget>
#include <QSizePolicy>
#include <QLabel>
#include <QFont>
#include <QFileDialog>

#include <cmath>

#include "math_functions.h"
#include "cabinet_edit_widget.h"
#include "nonlinear_widget.h"

CabinetEditWidget::CabinetEditWidget(QWidget *parent, Processor *prc, Player *plr) :
  BlockEditWidget(parent)
{
  processor = prc;
  player = plr;
  setFrameShape(QFrame::Panel);

  QVBoxLayout *vbox = new QVBoxLayout(this);

  QLabel *cabinetEqualizerLabel = new QLabel(tr("Equalizer"), this);
  vbox->addWidget(cabinetEqualizerLabel);
  cabinetEqualizerLabel->setMaximumHeight(30);
  cabinetEqualizerLabel->setAlignment(Qt::AlignHCenter);
  QFont cabinetEqualizerLabelFont = cabinetEqualizerLabel->font();
  cabinetEqualizerLabelFont.setPointSize(15);
  cabinetEqualizerLabel->setFont(cabinetEqualizerLabelFont);

  QWidget *equalizerButtonsBar = new QWidget(this);
  vbox->addWidget(equalizerButtonsBar);
  equalizerButtonsBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  QHBoxLayout *equalizerButtonsHBox = new QHBoxLayout(equalizerButtonsBar);

  loadButton = new QPushButton(tr("Load from file"), equalizerButtonsBar);
  equalizerButtonsHBox->addWidget(loadButton);

  connect(loadButton, &QPushButton::clicked, this, &CabinetEditWidget::loadButtonClicked);

  saveButton = new QPushButton(tr("Save to file"), equalizerButtonsBar);
  equalizerButtonsHBox->addWidget(saveButton);

  connect(saveButton, &QPushButton::clicked, this, &CabinetEditWidget::saveButtonClicked);

  resetButton = new QPushButton(tr("Reset"), equalizerButtonsBar);
  equalizerButtonsHBox->addWidget(resetButton);

  connect(resetButton, &QPushButton::clicked, this, &CabinetEditWidget::resetButtonClicked);

  applyButton = new QPushButton(tr("Apply"), equalizerButtonsBar);
  equalizerButtonsHBox->addWidget(applyButton);

  connect(applyButton, &QPushButton::clicked, this, &CabinetEditWidget::applyButtonClicked);

  equalizerButtonsHBox->addSpacing(20);

  autoEqButton = new QPushButton(tr("Auto Equalizer"), equalizerButtonsBar);
  equalizerButtonsHBox->addWidget(autoEqButton);

  connect(autoEqButton, &QPushButton::clicked, this, &CabinetEditWidget::autoEqButtonClicked);

  equalizerButtonsHBox->addSpacing(40);

  disableButton = new QPushButton(equalizerButtonsBar);
  equalizerButtonsHBox->addWidget(disableButton);

  if (processor->isCabinetCorrectionEnabled())
  {
    disableStatus = STAT_ENABLED;
    disableButton->setText(tr("Disable"));
  }
  else
  {
    disableStatus = STAT_DISABLED;
    disableButton->setText(tr("Enable"));
  }

  connect(disableButton, &QPushButton::clicked, this, &CabinetEditWidget::disableButtonClicked);

  equalizer = new EqualizerWidget(this);
  vbox->addWidget(equalizer);

  connect(equalizer, &EqualizerWidget::responseChanged, this, &CabinetEditWidget::responseChanged);

  autoEqThread = new AutoEqThread();

  autoEqThread->player = player;
  autoEqThread->processor = processor;
  autoEqThread->equalizer = equalizer;

  connect(autoEqThread, &QThread::finished, this, &CabinetEditWidget::autoEqThreadFinished);
  connect(autoEqThread, &AutoEqThread::progressChanged,
    this, &CabinetEditWidget::autoEqThreadProgressChanged);

  fileResamplingThread = new FileResamplingThread();

  connect(fileResamplingThread, &QThread::finished, this,
   &CabinetEditWidget::fileResamplingThreadFinished);

  msg = new MessageWidget(this);

  if (processor->correctionEqualizerFLogValues.size() == 0)
  {
    processor->correctionEqualizerFLogValues = equalizer->fLogValuesEq;
  }
  else
  {
    equalizer->fLogValuesEq = processor->correctionEqualizerFLogValues;
  }

  if (processor->correctionEqualizerDbValues.size() == 0)
  {
    processor->correctionEqualizerDbValues = equalizer->dbValuesEq;
  }
  else
  {
    equalizer->dbValuesEq = processor->correctionEqualizerDbValues;
  }

  recalculate();
}

void CabinetEditWidget::recalculate()
{
  equalizer->fLogValuesEq = processor->correctionEqualizerFLogValues;
  equalizer->dbValuesEq = processor->correctionEqualizerDbValues;

  QVector<float> freqs(1000);
  for (int i = 0; i < freqs.size(); i++)
  {
    float fLog = (log10(20000.0) - log10(10.0))*(float)(i) / (freqs.size() - 1) + log10(10.0);
    freqs[i] = pow(10, fLog);
  }

  QVector<float> frequencyResponse = processor->getCabinetSumFrequencyResponse(freqs);

  equalizer->fLogValuesFr.resize(frequencyResponse.size());
  equalizer->dbValuesFr.resize(frequencyResponse.size());

  for (int i = 0; i < frequencyResponse.size(); i++)
  {
    equalizer->fLogValuesFr[i] = log10(freqs[i]);
    equalizer->dbValuesFr[i] = 20.0 * log10(frequencyResponse[i]);
  }

  equalizer->drawBackground();
  equalizer->update(0,0,width(),height());
}

void CabinetEditWidget::responseChanged()
{
  processor->correctionEqualizerFLogValues = equalizer->fLogValuesEq;
  processor->correctionEqualizerDbValues = equalizer->dbValuesEq;

  QVector<double> w(equalizer->fLogValuesEq.size());
  QVector<double> A(equalizer->fLogValuesEq.size());

  for (int i = 0; i < w.size(); i++)
  {
    w[i] = 2.0 * M_PI * pow(10.0, equalizer->fLogValuesEq[i]);
    A[i] = pow(10.0, equalizer->dbValuesEq[i] / 20.0);
  }

  disableStatus = STAT_ENABLED;
  disableButton->setText(tr("Disable"));

  processor->setCabinetSumCorrectionImpulseFromFrequencyResponse(w, A);
}

void CabinetEditWidget::applyButtonClicked()
{
  processor->applyCabinetSumCorrection();
  recalculate();
  resetButtonClicked();
}

void CabinetEditWidget::resetButtonClicked()
{
  processor->resetCabinetSumCorrection();

  equalizer->resetEq();

  processor->correctionEqualizerFLogValues = equalizer->fLogValuesEq;
  processor->correctionEqualizerDbValues = equalizer->dbValuesEq;

  equalizer->drawBackground();
  equalizer->update(0,0,width(),height());
}

void CabinetEditWidget::autoEqButtonClicked()
{
  msg->setProgressValue(0);

  if (player->refDataL.size() != 0)
  {
    msg->setMessage(tr("Analyzing..."));
    msg->setTitle(tr("Please Wait!"));

    msg->open();

    autoEqThread->start();
  }
  else
  {
    QMessageBox::information(this, tr("AutoEqualizer"), tr("Reference file is not loaded!"));
  }
}

void CabinetEditWidget::autoEqThreadFinished()
{
  msg->setProgressValue(100);
  msg->close();

  responseChanged();

  disableStatus = STAT_ENABLED;
  disableButton->setText(tr("Disable"));

  equalizer->drawBackground();
  equalizer->update(0, 0, width(), height());
}

void CabinetEditWidget::resetControls()
{
  resetButtonClicked();
}

void CabinetEditWidget::saveButtonClicked()
{
  QString saveFileName =
    QFileDialog::getSaveFileName(this,
                                tr("Save impulse response file"),
                                QString(),
                                tr("WAV files (*.wav)"));

  if (!saveFileName.isEmpty())
  {
    QVector<float> left_impulse = processor->getLeftImpulse();
    QVector<float> right_impulse = processor->getRightImpulse();

    SF_INFO sfinfo;
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    sfinfo.frames = left_impulse.size();
    sfinfo.samplerate = processor->getSamplingRate();
    sfinfo.channels = 2;
    sfinfo.sections = 1;
    sfinfo.seekable = 1;

    SNDFILE *impulseFile = sf_open(saveFileName.toUtf8().constData(), SFM_WRITE, &sfinfo);
    if (impulseFile != NULL)
    {
      QVector<float> tempBuffer(left_impulse.size() * 2);

      for (int i = 0; i < (left_impulse.size() * 2 - 1); i += 2)
      {
        tempBuffer[i] = left_impulse[i / 2];
        tempBuffer[i + 1] = right_impulse[i / 2];
      }

      sf_writef_float(impulseFile, tempBuffer.data(), left_impulse.size());
    }

    sf_close(impulseFile);
  }
}

void CabinetEditWidget::loadButtonClicked()
{
  QString filename = QFileDialog::getOpenFileName(this,
                                tr("Open impulse response file"),
                                QString(),
                                tr("Sound files (*.wav *.ogg *.flac)"));

  if (!filename.isEmpty())
  {
    msg->setMessage(tr("Resampling..."));
    msg->setTitle(tr("Please Wait!"));

    msg->open();

    fileResamplingThread->stereoMode = true;
    fileResamplingThread->filename = filename;
    fileResamplingThread->samplingRate = processor->getSamplingRate();

    fileResamplingThread->start();
  }
}

void CabinetEditWidget::fileResamplingThreadFinished()
{
  msg->setProgressValue(100);
  msg->close();

  float cabinetImpulseEnergy = 0.0;

  for (int i = 0; i < fileResamplingThread->dataL.size(); i++)
  {
    cabinetImpulseEnergy += pow(fileResamplingThread->dataL[i], 2);
  }

  float cabinetImpulseEnergyCoeff = sqrt(0.45 * 48000.0 /
    (float)processor->getSamplingRate()) /
    sqrt(cabinetImpulseEnergy);

  for (int i = 0; i < fileResamplingThread->dataL.size(); i++)
  {
    fileResamplingThread->dataL[i] *= cabinetImpulseEnergyCoeff;
    fileResamplingThread->dataR[i] *= cabinetImpulseEnergyCoeff;
  }

  processor->setCabinetImpulse(fileResamplingThread->dataL, fileResamplingThread->dataR);
  recalculate();
}


void CabinetEditWidget::disableButtonClicked()
{
  switch (disableStatus)
  {
    case STAT_DISABLED:
    {
      disableStatus = STAT_ENABLED;
      disableButton->setText(tr("Disable"));
      processor->setCabinetCorrectionStatus(true);
    }
    break;
    case STAT_ENABLED:
    {
      disableStatus = STAT_DISABLED;
      disableButton->setText(tr("Enable"));
      processor->setCabinetCorrectionStatus(false);
    }
  }
}

void CabinetEditWidget::autoEqThreadProgressChanged(int progress)
{
  msg->setProgressValue(progress);
}

void AutoEqThread::run()
{
  int size_divisible_by_fragm = floor((double)player->diData.size() / (double)fragm) * fragm;
  QVector<double> diData(size_divisible_by_fragm);

  for (int i = 0; i < diData.size(); i++)
  {
    diData[i] = player->diData[i];
  }

  QVector<double> refData(player->refDataL.size());

  for (int i = 0; i < refData.size(); i++)
  {
    refData[i] = (player->refDataL[i] + player->refDataR[i]) / 2.0;
  }

  QVector<float> floatProcessedDataL(diData.size());
  QVector<float> floatProcessedDataR(diData.size());

  QSharedPointer<Processor> backProcessor
    = QSharedPointer<Processor>(new Processor(processor->getSamplingRate()));

  backProcessor->loadProfile(processor->getProfileFileName());

  backProcessor->setControls(processor->getControls());
  backProcessor->setProfile(processor->getProfile());

  backProcessor->setPreampImpulse(processor->getPreampImpulse());
  backProcessor->setCabinetImpulse(processor->getLeftImpulse(), processor->getRightImpulse());

  QVector<double> w(processor->preampCorrectionEqualizerFLogValues.size());
  QVector<double> A(processor->preampCorrectionEqualizerFLogValues.size());

  for (int i = 0; i < w.size(); i++)
  {
    w[i] = 2.0 * M_PI * pow(10.0, processor->preampCorrectionEqualizerFLogValues[i]);
    A[i] = pow(10.0, processor->preampCorrectionEqualizerDbValues[i] / 20.0);
  }

  backProcessor->setPreampCorrectionImpulseFromFrequencyResponse(w, A);

  backProcessor->process(floatProcessedDataL.data(),
                         floatProcessedDataR.data(),
                         player->diData.data(),
                         floatProcessedDataL.size());

  emit progressChanged(30);

  QVector<double> processedData(floatProcessedDataL.size());

  for (int i = 0; i < floatProcessedDataL.size(); i++)
  {
    processedData[i] = (floatProcessedDataL[i] + floatProcessedDataR[i]) / 2.0;
  }

  int averageSpectrumSize = 4096;
  int autoEqualazierPointsNum = 40;

  equalizer->fLogValuesEq.resize(autoEqualazierPointsNum);
  equalizer->dbValuesEq.resize(autoEqualazierPointsNum);

  equalizer->fLogValuesEq[0] = log10(10.0);

  for (int i = 0; i < autoEqualazierPointsNum - 1; i++)
  {
    equalizer->fLogValuesEq[i + 1] = (log10(20000.0) - log10(10.0))
      * (double)(i + 1) / (equalizer->fLogValuesEq.size() - 1) + log10(10.0);
  }

  emit progressChanged(50);

  calulate_autoeq_amplitude_response(averageSpectrumSize,
                                     player->getSampleRate(),
                                     processedData.data(),
                                     processedData.size(),
                                     refData.data(),
                                     refData.size(),
                                     equalizer->fLogValuesEq.data(),
                                     equalizer->dbValuesEq.data(),
                                     autoEqualazierPointsNum
                                     );

  for (int i = 0; i < equalizer->fLogValuesEq.size(); i++)
  {
    if (equalizer->dbValuesEq[i] > 20.0)
    {
      equalizer->dbValuesEq[i] = 20.0;
    }

    if (equalizer->dbValuesEq[i] < (-30.0))
    {
      equalizer->dbValuesEq[i] = -30.0;
    }
  }
}
