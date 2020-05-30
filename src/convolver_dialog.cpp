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

#include "convolver_dialog.h"
#include "math_functions.h"

ConvolverDialog::ConvolverDialog(Processor *prc, QWidget *parent) : QDialog(parent)
{
  processor = prc;
  setMinimumWidth(600);
  setWindowTitle(tr("FFT Convolver"));

  QGridLayout *lay = new QGridLayout(this);

  QLabel *inputLabel = new QLabel(tr("Input Signal"), this);
  lay->addWidget(inputLabel, 0, 0, 1, 2);
  inputLabel->setAlignment(Qt::AlignCenter);

  QButtonGroup *inputGroup = new QButtonGroup(this);
  inputCabinetRadioButton = new QRadioButton(
    tr("Cabinet impulse response"), this);
  inputGroup->addButton(inputCabinetRadioButton);
  lay->addWidget(inputCabinetRadioButton, 1, 0, 1, 1);

  inputFileRadioButton = new QRadioButton(tr("File"), this);
  inputFileRadioButton->setChecked(true);
  inputGroup->addButton(inputFileRadioButton);
  lay->addWidget(inputFileRadioButton, 2, 0, 1, 1);

  connect(inputGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
    this, &ConvolverDialog::inputGroupClicked);

  inputFilenameEdit = new QLineEdit(this);
  lay->addWidget(inputFilenameEdit, 3, 0, 1, 1);

  inputFilenameButton = new QPushButton(tr("Open"), this);
  lay->addWidget(inputFilenameButton, 3, 1, 1, 1);

  connect(inputFilenameButton, &QPushButton::clicked,
    this, &ConvolverDialog::inputFilenameButtonClicked);

  QLabel *IRLabel = new QLabel(tr("Impulse response file"), this);
  lay->addWidget(IRLabel, 4, 0, 1, 2);
  IRLabel->setAlignment(Qt::AlignCenter);

  IRFilenameEdit = new QLineEdit(this);
  lay->addWidget(IRFilenameEdit, 5, 0, 1, 1);

  QPushButton *IRFilenameButton = new QPushButton(tr("Open"), this);
  lay->addWidget(IRFilenameButton, 5, 1, 1, 1);

  connect(IRFilenameButton, &QPushButton::clicked,
    this, &ConvolverDialog::IRFilenameButtonClicked);

  QLabel *outputLabel = new QLabel(tr("Output Signal"), this);
  lay->addWidget(outputLabel, 6, 0, 1, 2);
  outputLabel->setAlignment(Qt::AlignCenter);

  QButtonGroup *outputGroup = new QButtonGroup(this);
  outputCabinetRadioButton = new QRadioButton(
    tr("Cabinet impulse response"), this);
  outputGroup->addButton(outputCabinetRadioButton);
  lay->addWidget(outputCabinetRadioButton, 7, 0, 1, 1);

  outputFileRadioButton = new QRadioButton(tr("File"), this);
  outputFileRadioButton->setChecked(true);
  outputGroup->addButton(outputFileRadioButton);
  lay->addWidget(outputFileRadioButton, 8, 0, 1, 1);

  connect(outputGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked),
    this, &ConvolverDialog::outputGroupClicked);

  outputFilenameEdit = new QLineEdit(this);
  lay->addWidget(outputFilenameEdit, 9, 0, 1, 1);

  outputFilenameButton = new QPushButton(tr("Save"), this);
  lay->addWidget(outputFilenameButton, 9, 1, 1, 1);

  connect(outputFilenameButton, &QPushButton::clicked,
    this, &ConvolverDialog::outputFilenameButtonClicked);

  QWidget *buttonsContainer = new QWidget(this);
  lay->addWidget(buttonsContainer, 10, 0, 1, 2);

  QHBoxLayout *containerLay = new QHBoxLayout(buttonsContainer);
  processButton = new QPushButton(tr("Process"), buttonsContainer);
  containerLay->addWidget(processButton);
  processButton->setMaximumWidth(80);
  processButton->setEnabled(false);

  connect(processButton, &QPushButton::clicked,
    this, &ConvolverDialog::processButtonClicked);

  QPushButton *closeButton = new QPushButton(tr("Close"), buttonsContainer);
  containerLay->addWidget(closeButton);
  closeButton->setMaximumWidth(80);

  connect(closeButton, &QPushButton::clicked, this,
    &ConvolverDialog::closeButtonClicked);
}

void ConvolverDialog::inputFilenameButtonClicked()
{
  QString inputFileName = QFileDialog::getOpenFileName(this,
    tr("Open Input File"), QString(), tr("Sound files (*.wav *.ogg *.flac)"));

  if (!inputFileName.isEmpty())
  {
    inputFilenameEdit->setText(inputFileName);
  }

  checkSignals();
}

void ConvolverDialog::outputFilenameButtonClicked()
{
  QString outputFileName = QFileDialog::getSaveFileName(this,
    tr("Save Output File"), QString(), tr("Sound files (*.wav *.ogg *.flac)"));

  if (!outputFileName.isEmpty())
  {
    outputFilenameEdit->setText(outputFileName);
  }

  checkSignals();
}

void ConvolverDialog::IRFilenameButtonClicked()
{
  QString IRFileName = QFileDialog::getOpenFileName(this,
    tr("Open IR File"), QString(), tr("Sound files (*.wav *.ogg *.flac)"));

  if (!IRFileName.isEmpty())
  {
    IRFilenameEdit->setText(IRFileName);
  }

  checkSignals();
}

void ConvolverDialog::processButtonClicked()
{
  QString inputFileName = inputFilenameEdit->text();
  QString outputFileName = outputFilenameEdit->text();
  QString IRFileName = IRFilenameEdit->text();

  QVector<float> inputL;
  QVector<float> inputR;

  int inputSampleRate;

  SF_INFO sfinfo;
  SNDFILE *sndFile;

  if (inputFileRadioButton->isChecked())
  {
    sfinfo.format = 0;

    sndFile = sf_open(inputFileName.toUtf8().constData(), SFM_READ, &sfinfo);
    if (sndFile != NULL)
    {
      inputSampleRate = sfinfo.samplerate;

      QVector<float> tempBuffer(sfinfo.frames * sfinfo.channels);
      sf_readf_float(sndFile, tempBuffer.data(), sfinfo.frames);

      sf_close(sndFile);

      inputL.resize(sfinfo.frames);
      inputR.resize(sfinfo.frames);

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
          inputL[i / sfinfo.channels] = tempBuffer[i];
          inputR[i / sfinfo.channels] = sumFrame;
        }
        else
        {
          inputL[i] = tempBuffer[i];
          inputR[i] = tempBuffer[i];
        }
      }
    }
    else
    {
      return;
    }
  }
  else
  {
    inputL = processor->getLeftImpulse();
    inputR = processor->getRightImpulse();
    inputSampleRate = processor->getSamplingRate();
  }

  sfinfo.format = 0;

  QVector<float> IRL;
  QVector<float> IRR;

  int IRSampleRate;

  sndFile = sf_open(IRFileName.toUtf8().constData(), SFM_READ, &sfinfo);
  if (sndFile != NULL)
  {
    IRSampleRate = sfinfo.samplerate;

    QVector<float> tempBuffer(sfinfo.frames * sfinfo.channels);
    sf_readf_float(sndFile, tempBuffer.data(), sfinfo.frames);

    sf_close(sndFile);

    IRL.resize(sfinfo.frames);
    IRR.resize(sfinfo.frames);

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
        IRL[i / sfinfo.channels] = tempBuffer[i];
        IRR[i / sfinfo.channels] = sumFrame;
      }
      else
      {
        IRL[i] = tempBuffer[i];
        IRR[i] = tempBuffer[i];
      }
    }
  }
  else
  {
    return;
  }

  int outputSampleRate;

  if (inputSampleRate >= IRSampleRate)
  {
    outputSampleRate = inputSampleRate;
  }
  else
  {
    outputSampleRate = IRSampleRate;
  }

  inputL = resample_vector(inputL, inputSampleRate, outputSampleRate);
  inputR = resample_vector(inputR, inputSampleRate, outputSampleRate);
  IRL = resample_vector(IRL, IRSampleRate, outputSampleRate);
  IRR = resample_vector(IRR, IRSampleRate, outputSampleRate);

  float loadedCabinetImpulseEnergy = 0.0;

  for (int i = 0; i < inputL.size(); i++)
  {
    loadedCabinetImpulseEnergy += pow(inputL[i], 2);
  }

  fft_convolver(inputL.data(), inputL.size(),
    IRL.data(), IRL.size());

  fft_convolver(inputR.data(), inputR.size(),
    IRR.data(), IRR.size());

  float cabinetImpulseEnergy = 0.0;

  for (int i = 0; i < inputL.size(); i++)
  {
    cabinetImpulseEnergy += pow(inputL[i], 2);
  }

  float cabinetImpulseEnergyCoeff = sqrt(loadedCabinetImpulseEnergy) /
    sqrt(cabinetImpulseEnergy);

  for (int i = 0; i < inputL.size(); i++)
  {
    inputL[i] *= cabinetImpulseEnergyCoeff;
    inputR[i] *= cabinetImpulseEnergyCoeff;
  }

  if (outputFileRadioButton->isChecked())
  {
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    sfinfo.frames = inputL.size();
    sfinfo.samplerate = outputSampleRate;
    sfinfo.channels = 2;
    sfinfo.sections = 1;
    sfinfo.seekable = 1;

    SNDFILE *outputFile = sf_open(outputFileName.toUtf8().constData(),
      SFM_WRITE, &sfinfo);

    if (outputFile != NULL)
    {
      QVector<float> tempBuffer(inputL.size() * 2);

      for (int i = 0; i < (inputL.size() * 2 - 1); i += 2)
      {
        tempBuffer[i] = inputL[i / 2];
        tempBuffer[i + 1] = inputR[i / 2];
      }

      sf_writef_float(outputFile, tempBuffer.data(), inputL.size());
      sf_close(outputFile);
    }
  }
  else
  {
    processor->setCabinetImpulse(inputL, inputR);
  }
}

void ConvolverDialog::closeButtonClicked()
{
  close();
}

void ConvolverDialog::inputGroupClicked(QAbstractButton *button)
{
  if (button == inputCabinetRadioButton)
  {
    inputFilenameEdit->setEnabled(false);
    inputFilenameButton->setEnabled(false);
  }
  else
  {
    inputFilenameEdit->setEnabled(true);
    inputFilenameButton->setEnabled(true);
  }

  checkSignals();
}

void ConvolverDialog::outputGroupClicked(QAbstractButton *button)
{
  if (button == outputCabinetRadioButton)
  {
    outputFilenameEdit->setEnabled(false);
    outputFilenameButton->setEnabled(false);
  }
  else
  {
    outputFilenameEdit->setEnabled(true);
    outputFilenameButton->setEnabled(true);
  }

  checkSignals();
}

void ConvolverDialog::checkSignals()
{
  if (!((inputFilenameEdit->text().isEmpty() && inputFileRadioButton->isChecked()) ||
    (outputFilenameEdit->text().isEmpty() && outputFileRadioButton->isChecked()) ||
    IRFilenameEdit->text().isEmpty()))
  {
    processButton->setEnabled(true);
  }
  else
  {
    processButton->setEnabled(false);
  }
}
