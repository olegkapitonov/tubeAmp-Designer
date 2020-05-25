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

#include "preamp_filter_edit_widget.h"
#include "nonlinear_widget.h"

PreampFilterEditWidget::PreampFilterEditWidget(QWidget *parent, Processor *prc) :
  BlockEditWidget(parent)
{
  processor = prc;
  setFrameShape(QFrame::Panel);

  QVBoxLayout *vbox = new QVBoxLayout(this);

  QLabel *preampFilterEqualizerLabel = new QLabel(tr("Equalizer"), this);
  vbox->addWidget(preampFilterEqualizerLabel);
  preampFilterEqualizerLabel->setMaximumHeight(30);
  preampFilterEqualizerLabel->setAlignment(Qt::AlignHCenter);
  QFont preampFilterEqualizerLabelFont = preampFilterEqualizerLabel->font();
  preampFilterEqualizerLabelFont.setPointSize(15);
  preampFilterEqualizerLabel->setFont(preampFilterEqualizerLabelFont);

  QWidget *equalizerButtonsBar = new QWidget(this);
  vbox->addWidget(equalizerButtonsBar);
  equalizerButtonsBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  QHBoxLayout *equalizerButtonsHBox = new QHBoxLayout(equalizerButtonsBar);

  loadButton = new QPushButton(tr("Load from file"), equalizerButtonsBar);
  equalizerButtonsHBox->addWidget(loadButton);

  connect(loadButton, &QPushButton::clicked, this,
          &PreampFilterEditWidget::loadButtonClicked);

  saveButton = new QPushButton(tr("Save to file"), equalizerButtonsBar);
  equalizerButtonsHBox->addWidget(saveButton);

  connect(saveButton, &QPushButton::clicked, this,
          &PreampFilterEditWidget::saveButtonClicked);

  resetButton = new QPushButton(tr("Reset"), equalizerButtonsBar);
  equalizerButtonsHBox->addWidget(resetButton);

  connect(resetButton, &QPushButton::clicked, this,
          &PreampFilterEditWidget::resetButtonClicked);

  applyButton = new QPushButton(tr("Apply"), equalizerButtonsBar);
  equalizerButtonsHBox->addWidget(applyButton);

  connect(applyButton, &QPushButton::clicked, this,
          &PreampFilterEditWidget::applyButtonClicked);

  equalizerButtonsHBox->addSpacing(40);

  disableButton = new QPushButton(equalizerButtonsBar);
  equalizerButtonsHBox->addWidget(disableButton);

  if (processor->isPreampCorrectionEnabled())
  {
    disableStatus = STAT_ENABLED;
    disableButton->setText(tr("Disable"));
  }
  else
  {
    disableStatus = STAT_DISABLED;
    disableButton->setText(tr("Enable"));
  }

  connect(disableButton, &QPushButton::clicked, this,
          &PreampFilterEditWidget::disableButtonClicked);

  equalizer = new EqualizerWidget(this);
  vbox->addWidget(equalizer);

  connect(equalizer, &EqualizerWidget::responseChanged, this,
          &PreampFilterEditWidget::responseChangedSlot);

  fileResamplingThread = new FileResamplingThread();

  connect(fileResamplingThread, &QThread::finished, this,
   &PreampFilterEditWidget::fileResamplingThreadFinished);

  if (processor->preampCorrectionEqualizerFLogValues.size() == 0)
  {
    processor->preampCorrectionEqualizerFLogValues = equalizer->fLogValuesEq;
  }
  else
  {
    equalizer->fLogValuesEq = processor->preampCorrectionEqualizerFLogValues;
  }

  if (processor->preampCorrectionEqualizerDbValues.size() == 0)
  {
    processor->preampCorrectionEqualizerDbValues = equalizer->dbValuesEq;
  }
  else
  {
    equalizer->dbValuesEq = processor->preampCorrectionEqualizerDbValues;
  }

  msg = new MessageWidget(this);

  recalculate();
}

void PreampFilterEditWidget::recalculate()
{
  equalizer->fLogValuesEq = processor->preampCorrectionEqualizerFLogValues;
  equalizer->dbValuesEq = processor->preampCorrectionEqualizerDbValues;

  QVector<float> freqs(100);
  for (int i = 0; i < freqs.size(); i++)
  {
    float fLog = (log10(20000.0) - log10(10.0))*(float)(i) / 99.0 + log10(10.0);
    freqs[i] = pow(10, fLog);
  }

  QVector<float> frequencyResponse = processor->getPreampFrequencyResponse(freqs);

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

void PreampFilterEditWidget::responseChangedSlot()
{
  processor->preampCorrectionEqualizerFLogValues = equalizer->fLogValuesEq;
  processor->preampCorrectionEqualizerDbValues = equalizer->dbValuesEq;

  QVector<double> w(equalizer->fLogValuesEq.size());
  QVector<double> A(equalizer->fLogValuesEq.size());

  for (int i = 0; i < w.size(); i++)
  {
    w[i] = 2.0 * M_PI * pow(10.0, equalizer->fLogValuesEq[i]);
    A[i] = pow(10.0, equalizer->dbValuesEq[i] / 20.0);
  }

  disableStatus = STAT_ENABLED;
  disableButton->setText(tr("Disable"));

  processor->setPreampCorrectionImpulseFromFrequencyResponse(w, A);
}

void PreampFilterEditWidget::applyButtonClicked()
{
  processor->applyPreampCorrection();
  recalculate();

  resetButtonClicked();
}

void PreampFilterEditWidget::resetButtonClicked()
{
  processor->resetPreampCorrection();

  equalizer->resetEq();

  processor->preampCorrectionEqualizerFLogValues = equalizer->fLogValuesEq;
  processor->preampCorrectionEqualizerDbValues = equalizer->dbValuesEq;

  equalizer->drawBackground();
  equalizer->update(0,0,width(),height());
}

void PreampFilterEditWidget::resetControls()
{
  resetButtonClicked();
}

void PreampFilterEditWidget::saveButtonClicked()
{
  QString saveFileName =
    QFileDialog::getSaveFileName(this,
                                tr("Save impulse response file"),
                                QString(),
                                tr("WAV files (*.wav)"));

  if (!saveFileName.isEmpty())
  {
    QVector<float> preamp_impulse = processor->getPreampImpulse();

    SF_INFO sfinfo;
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    sfinfo.frames = preamp_impulse.size();
    sfinfo.samplerate = processor->getSamplingRate();
    sfinfo.channels = 1;
    sfinfo.sections = 1;
    sfinfo.seekable = 1;

    SNDFILE *impulseFile = sf_open(saveFileName.toUtf8().constData(), SFM_WRITE, &sfinfo);

    if (impulseFile != NULL)
    {
      sf_writef_float(impulseFile, preamp_impulse.data(), preamp_impulse.size());
    }

    sf_close(impulseFile);
  }
}

void PreampFilterEditWidget::loadButtonClicked()
{
  QString filename = QFileDialog::getOpenFileName(this,
                                tr("Open impulse response file"),
                                QString(),
                                tr("Sound files (*.wav *.ogg *.flac)"));

  if (!filename.isEmpty())
  {
    msg->setTitle(tr("Please wait!"));
    msg->setMessage(tr("Resampling impulse response file..."));
    msg->open();

    fileResamplingThread->filename = filename;
    fileResamplingThread->samplingRate = processor->getSamplingRate();

    fileResamplingThread->start();
  }
}

void PreampFilterEditWidget::fileResamplingThreadFinished()
{
  msg->setProgressValue(100);
  msg->close();

  float max_val = 0.0;

  for (int i = 0; i < fileResamplingThread->dataL.size(); i++)
  {
    if (fabs(fileResamplingThread->dataL[i]) > max_val)
    {
      max_val = fabs(fileResamplingThread->dataL[i]);
    }
  }

  max_val /= 0.4 * (48000.0 / (float)processor->getSamplingRate());

  for (int i = 0; i < fileResamplingThread->dataL.size(); i++)
  {
    fileResamplingThread->dataL[i] /= max_val;
  }

  processor->setPreampImpulse(fileResamplingThread->dataL);
  recalculate();
}

void PreampFilterEditWidget::disableButtonClicked()
{
  switch (disableStatus)
  {
    case STAT_DISABLED:
    {
      disableStatus = STAT_ENABLED;
      disableButton->setText(tr("Disable"));
      processor->setPreampCorrectionStatus(true);
    }
    break;
    case STAT_ENABLED:
    {
      disableStatus = STAT_DISABLED;
      disableButton->setText(tr("Enable"));
      processor->setPreampCorrectionStatus(false);
    }
  }
}
