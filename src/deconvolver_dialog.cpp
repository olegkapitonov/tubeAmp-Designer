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

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>

#include <sndfile.h>
#include <cmath>

#include "deconvolver_dialog.h"
#include "math_functions.h"

DeconvolverDialog::DeconvolverDialog(Processor *prc, QWidget *parent) : QDialog(parent)
{
  processor = prc;
  setMinimumWidth(600);
  setWindowTitle(tr("FFT Deconvolver"));

  QGridLayout *lay = new QGridLayout(this);

  QPushButton *saveTestSignalButton = new QPushButton(tr("Create test signal .wav"), this);
  lay->addWidget(saveTestSignalButton, 0, 0, 1, 2);
  connect(saveTestSignalButton, &QPushButton::clicked, this,
    &DeconvolverDialog::saveTestSignalButtonClicked);

  QLabel *testLabel = new QLabel(tr("Test Signal"), this);
  lay->addWidget(testLabel, 1, 0, 1, 2);
  testLabel->setAlignment(Qt::AlignCenter);

  testFilenameEdit = new QLineEdit(this);
  lay->addWidget(testFilenameEdit, 2, 0, 1, 1);

  testFilenameButton = new QPushButton(tr("Open"), this);
  lay->addWidget(testFilenameButton, 2, 1, 1, 1);

  connect(testFilenameButton, &QPushButton::clicked,
    this, &DeconvolverDialog::testFilenameButtonClicked);

  QLabel *responseLabel = new QLabel(tr("Response Signal"), this);
  lay->addWidget(responseLabel, 3, 0, 1, 2);
  responseLabel->setAlignment(Qt::AlignCenter);

  responseFilenameEdit = new QLineEdit(this);
  lay->addWidget(responseFilenameEdit, 4, 0, 1, 1);

  responseFilenameButton = new QPushButton(tr("Open"), this);
  lay->addWidget(responseFilenameButton, 4, 1, 1, 1);

  connect(responseFilenameButton, &QPushButton::clicked,
    this, &DeconvolverDialog::responseFilenameButtonClicked);

  QLabel *IRLabel = new QLabel(tr("Impulse response file"), this);
  lay->addWidget(IRLabel, 5, 0, 1, 2);
  IRLabel->setAlignment(Qt::AlignCenter);

  QButtonGroup *IRGroup = new QButtonGroup(this);
  IRCabinetRadioButton = new QRadioButton(
    tr("Cabinet impulse response"), this);
  IRGroup->addButton(IRCabinetRadioButton);
  lay->addWidget(IRCabinetRadioButton, 6, 0, 1, 1);

  IRFileRadioButton = new QRadioButton(tr("File"), this);
  IRFileRadioButton->setChecked(true);
  IRGroup->addButton(IRFileRadioButton);
  lay->addWidget(IRFileRadioButton, 7, 0, 1, 1);

  connect(IRGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
    this, &DeconvolverDialog::IRGroupClicked);

  IRFilenameEdit = new QLineEdit(this);
  lay->addWidget(IRFilenameEdit, 8, 0, 1, 1);

  IRFilenameButton = new QPushButton(tr("Save"), this);
  lay->addWidget(IRFilenameButton, 8, 1, 1, 1);

  connect(IRFilenameButton, &QPushButton::clicked,
    this, &DeconvolverDialog::IRFilenameButtonClicked);

  QWidget *buttonsContainer = new QWidget(this);
  lay->addWidget(buttonsContainer, 9, 0, 1, 2);

  QHBoxLayout *containerLay = new QHBoxLayout(buttonsContainer);
  processButton = new QPushButton(tr("Process"), buttonsContainer);
  containerLay->addWidget(processButton);
  processButton->setMaximumWidth(80);
  processButton->setEnabled(false);

  connect(processButton, &QPushButton::clicked,
    this, &DeconvolverDialog::processButtonClicked);

  QPushButton *closeButton = new QPushButton(tr("Close"), buttonsContainer);
  containerLay->addWidget(closeButton);
  closeButton->setMaximumWidth(80);

  connect(closeButton, &QPushButton::clicked, this,
    &DeconvolverDialog::closeButtonClicked);
}

void DeconvolverDialog::testFilenameButtonClicked()
{
  QString testFileName = QFileDialog::getOpenFileName(this,
    tr("Open Test Signal File"), QString(), tr("Sound files (*.wav *.ogg *.flac)"));

  if (!testFileName.isEmpty())
  {
    testFilenameEdit->setText(testFileName);
  }

  checkSignals();
}

void DeconvolverDialog::responseFilenameButtonClicked()
{
  QString responseFileName = QFileDialog::getOpenFileName(this,
    tr("Open Response File"), QString(), tr("Sound files (*.wav *.ogg *.flac)"));

  if (!responseFileName.isEmpty())
  {
    responseFilenameEdit->setText(responseFileName);
  }

  checkSignals();
}

void DeconvolverDialog::IRFilenameButtonClicked()
{
  QString IRFileName = QFileDialog::getOpenFileName(this,
    tr("Open IR File"), QString(), tr("Sound files (*.wav *.ogg *.flac)"));

  if (!IRFileName.isEmpty())
  {
    IRFilenameEdit->setText(IRFileName);
  }

  checkSignals();
}

void DeconvolverDialog::processButtonClicked()
{
  QString testFileName = testFilenameEdit->text();
  QString responseFileName = responseFilenameEdit->text();
  QString IRFileName = IRFilenameEdit->text();

  QVector<float> testL;
  QVector<float> testR;

  int testSampleRate;

  SF_INFO sfinfo;
  SNDFILE *sndFile;

  sfinfo.format = 0;

  sndFile = sf_open(testFileName.toUtf8().constData(), SFM_READ, &sfinfo);
  if (sndFile != NULL)
  {
    testSampleRate = sfinfo.samplerate;

    QVector<float> tempBuffer(sfinfo.frames * sfinfo.channels);
    sf_readf_float(sndFile, tempBuffer.data(), sfinfo.frames);

    sf_close(sndFile);

    testL.resize(sfinfo.frames);
    testR.resize(sfinfo.frames);

    for (int i = 0; i < sfinfo.frames * sfinfo.channels; i += sfinfo.channels)
    {
      float sumFrame = 0.0;
      if (sfinfo.channels > 1)
      {
        for (int j = 1; j < sfinfo.channels; j++)
        {
          sumFrame += tempBuffer[i + j];
        }
        sumFrame /= sfinfo.channels - 1;
        testL[i / sfinfo.channels] = tempBuffer[i];
        testR[i / sfinfo.channels] = sumFrame;
      }
      else
      {
        testL[i] = tempBuffer[i];
        testR[i] = tempBuffer[i];
      }
    }
  }

  sfinfo.format = 0;

  QVector<float> responseL;
  QVector<float> responseR;

  int responseSampleRate;

  sndFile = sf_open(responseFileName.toUtf8().constData(), SFM_READ, &sfinfo);
  if (sndFile != NULL)
  {
    responseSampleRate = sfinfo.samplerate;

    QVector<float> tempBuffer(sfinfo.frames * sfinfo.channels);
    sf_readf_float(sndFile, tempBuffer.data(), sfinfo.frames);

    sf_close(sndFile);

    responseL.resize(sfinfo.frames);
    responseR.resize(sfinfo.frames);

    for (int i = 0; i < sfinfo.frames * sfinfo.channels; i += sfinfo.channels)
    {
      float sumFrame = 0.0;
      if (sfinfo.channels > 1)
      {
        for (int j = 1; j < sfinfo.channels; j++)
        {
          sumFrame += tempBuffer[i + j];
        }
        sumFrame /= sfinfo.channels - 1;
        responseL[i / sfinfo.channels] = tempBuffer[i];
        responseR[i / sfinfo.channels] = sumFrame;
      }
      else
      {
        responseL[i] = tempBuffer[i];
        responseR[i] = tempBuffer[i];
      }
    }
  }

  int IRSampleRate;

  if (testSampleRate >= responseSampleRate)
  {
    IRSampleRate = testSampleRate;
  }
  else
  {
    IRSampleRate = responseSampleRate;
  }

  testL = resample_vector(testL, testSampleRate, IRSampleRate);
  testR = resample_vector(testR, testSampleRate, IRSampleRate);
  responseL = resample_vector(responseL, responseSampleRate, IRSampleRate);
  responseR = resample_vector(responseR, responseSampleRate, IRSampleRate);

  QVector<float> IRL(responseL.size());
  QVector<float> IRR(responseR.size());

  fft_deconvolver(testL.data(),
                  testL.size(),
                  responseL.data(),
                  responseL.size(),
                  IRL.data(),
                  IRL.size(),
                  20.0 / processor->getSamplingRate(),
                  20000.0 / processor->getSamplingRate(),
                  -60.0
                 );

  fft_deconvolver(testR.data(),
                  testR.size(),
                  responseR.data(),
                  responseR.size(),
                  IRR.data(),
                  IRR.size(),
                  20.0 / processor->getSamplingRate(),
                  20000.0 / processor->getSamplingRate(),
                  -60.0
                 );

  float cabinetImpulseEnergy = 0.0;

  for (int i = 0; i < IRL.size(); i++)
  {
    cabinetImpulseEnergy += pow(IRL[i], 2);
  }

  float cabinetImpulseEnergyCoeff = sqrt(0.45 * 48000.0 /
    (float)processor->getSamplingRate()) /
    sqrt(cabinetImpulseEnergy);

  for (int i = 0; i < IRL.size(); i++)
  {
    IRL[i] *= cabinetImpulseEnergyCoeff;
    IRR[i] *= cabinetImpulseEnergyCoeff;
  }

  if (IRFileRadioButton->isChecked())
  {
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    sfinfo.frames = IRL.size();
    sfinfo.samplerate = IRSampleRate;
    sfinfo.channels = 2;
    sfinfo.sections = 1;
    sfinfo.seekable = 1;

    SNDFILE *outputFile = sf_open(IRFileName.toUtf8().constData(),
      SFM_WRITE, &sfinfo);

    if (outputFile != NULL)
    {
      printf("saving\n");
      QVector<float> tempBuffer(IRL.size() * 2);

      for (int i = 0; i < (IRL.size() * 2 - 1); i += 2)
      {
        tempBuffer[i] = IRL[i / 2];
        tempBuffer[i + 1] = IRR[i / 2];
      }

      sf_writef_float(outputFile, tempBuffer.data(), IRL.size());
      sf_close(outputFile);
    }
  }
  else
  {
    processor->setCabinetImpulse(IRL, IRR);
  }
}

void DeconvolverDialog::closeButtonClicked()
{
  close();
}

void DeconvolverDialog::IRGroupClicked(QAbstractButton *button)
{
  if (button == IRCabinetRadioButton)
  {
    IRFilenameEdit->setEnabled(false);
    IRFilenameButton->setEnabled(false);
  }
  else
  {
    IRFilenameEdit->setEnabled(true);
    IRFilenameButton->setEnabled(true);
  }

  checkSignals();
}

void DeconvolverDialog::checkSignals()
{
  if (!(testFilenameEdit->text().isEmpty() ||
    responseFilenameEdit->text().isEmpty() ||
    (IRFilenameEdit->text().isEmpty() && IRFileRadioButton->isChecked())))
  {
    processButton->setEnabled(true);
  }
  else
  {
    processButton->setEnabled(false);
  }
}

void DeconvolverDialog::saveTestSignalButtonClicked()
{
  QString testFileName = QFileDialog::getSaveFileName(this,
    tr("Test Signal File"), QString(), tr("Sound files (*.wav *.ogg *.flac)"));

  if (!testFileName.isEmpty())
  {
    QVector<float> testSignal(10.0 * processor->getSamplingRate());

    generate_logarithmic_sweep(10.0, processor->getSamplingRate(), 20.0,
                               (float)processor->getSamplingRate() / 2.0,
                               0.5, testSignal.data());

    SF_INFO sfinfo;

    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    sfinfo.frames = testSignal.size();
    sfinfo.samplerate = processor->getSamplingRate();
    sfinfo.channels = 1;
    sfinfo.sections = 1;
    sfinfo.seekable = 1;

    SNDFILE *outputFile = sf_open(testFileName.toUtf8().constData(),
      SFM_WRITE, &sfinfo);

    if (outputFile != NULL)
    {
      sf_writef_float(outputFile, testSignal.data(), testSignal.size());
      sf_close(outputFile);
    }
  }
}
