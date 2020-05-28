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
#include <QVBoxLayout>
#include <QLabel>
#include <QFileDialog>

#include "profiler_dialog.h"

ProfilerDialog::ProfilerDialog(Processor *prc, Player *plr, PlayerPanel *pnl, QWidget *parent) : QDialog(parent)
{
  processor = prc;
  player = plr;
  playerPanel = pnl;
  setMinimumWidth(600);
  setWindowTitle(tr("Profiler"));

  QGridLayout *lay = new QGridLayout(this);

  QLabel *testSignalLabel = new QLabel(tr("Test signal"), this);
  lay->addWidget(testSignalLabel, 0, 0, 1, 1);

  testSignalComboBox = new QComboBox(this);
  testSignalComboBox->addItem(tr("Test Signal v1.0"));
  lay->addWidget(testSignalComboBox, 0, 1, 1, 1);

  createTestSignalWavButton = new QPushButton(tr("Create *.wav"), this);
  lay->addWidget(createTestSignalWavButton, 0, 2, 1, 1);

  connect(createTestSignalWavButton, &QPushButton::clicked, this,
    &ProfilerDialog::createTestSignalWavButtonClick);

  QLabel *responseFileLabel = new QLabel(tr("Response File"), this);
  lay->addWidget(responseFileLabel, 1, 0, 1, 1);

  responseFileEdit = new QLineEdit(this);
  lay->addWidget(responseFileEdit, 1, 1, 1, 1);

  responseFileOpenButton = new QPushButton(tr("Open"), this);
  lay->addWidget(responseFileOpenButton, 1, 2, 1, 1);

  connect(responseFileOpenButton, &QPushButton::clicked, this,
          &ProfilerDialog::responseFileOpenButtonClick);

  presetGroupBox = new QGroupBox(tr("Preset"), this);
  lay->addWidget(presetGroupBox, 2, 0, 1, 3);

  QVBoxLayout *presetLay = new QVBoxLayout(presetGroupBox);

  crystalcleanPresetRadioButton = new QRadioButton(tr("Crystal Clean"),
                                              presetGroupBox);
  presetLay->addWidget(crystalcleanPresetRadioButton);

  classicPresetRadioButton = new QRadioButton(tr("Classic (without master gain)"),
                                              presetGroupBox);
  presetLay->addWidget(classicPresetRadioButton);

  mastergainPresetRadioButton = new QRadioButton(tr("Master gain"), presetGroupBox);
  presetLay->addWidget(mastergainPresetRadioButton);

  classicPresetRadioButton->setChecked(true);

  analyzeButton = new QPushButton(tr("Analyze"), this);
  analyzeButton->setEnabled(false);
  lay->addWidget(analyzeButton, 3, 1, 1, 1);

  connect(analyzeButton, &QPushButton::clicked, this, &ProfilerDialog::analyzeButtonClick);

  cancelButton = new QPushButton(tr("Cancel"), this);
  lay->addWidget(cancelButton, 3, 2, 1, 1);

  connect(cancelButton, &QPushButton::clicked, this, &ProfilerDialog::cancelButtonClick);

  profilerThread = new ProfilerThread();

  connect(profilerThread, &QThread::finished, this,
   &ProfilerDialog::profilerThreadFinished);

  msg = new MessageWidget(this);
}

void ProfilerDialog::analyzeButtonClick()
{
  if (!responseFileEdit->text().isEmpty())
  {
    profiler = new Profiler(processor, player);

    connect(profiler, &Profiler::progressChanged, this,
      &ProfilerDialog::profilerProgressChanged);

    connect(profiler, &Profiler::stopPlaybackNeeded, playerPanel, &PlayerPanel::stopPlayback);

    profiler->loadResponseFile(responseFileEdit->text());

    profilerThread->profiler = profiler;

    if (crystalcleanPresetRadioButton->isChecked())
    {
      profilerThread->presetType = CRYSTALCLEAN_PRESET;
    }

    if (classicPresetRadioButton->isChecked())
    {
      profilerThread->presetType = CLASSIC_PRESET;
    }

    if (mastergainPresetRadioButton->isChecked())
    {
      profilerThread->presetType = MASTERGAIN_PRESET;
    }

    msg->setMessage(tr("Analyzing..."));
    msg->setTitle(tr("Please Wait!"));
    msg->setProgressValue(0);

    msg->open();

    profilerThread->start();
  }
}

void ProfilerDialog::responseFileOpenButtonClick()
{
  responseFileName = QFileDialog::getOpenFileName(this,
                                                  tr("Open Response File"),
                                                  QString(),
                                                  tr("Sound File (*.wav)"));

  if (!responseFileName.isEmpty())
  {
    responseFileEdit->setText(responseFileName);
  }

  if (!(responseFileName.isEmpty()))
  {
    analyzeButton->setEnabled(true);
  }
}

void ProfilerDialog::cancelButtonClick()
{
  reject();
}

void ProfilerDialog::createTestSignalWavButtonClick()
{
  QString testFileName = QFileDialog::getSaveFileName(this,
                                              tr("Create Test File"),
                                              QString(),
                                              tr("Sound File (*.wav)"));

  if (!(testFileName.isEmpty()))
  {
    Profiler profiler(processor, player);
    profiler.createTestFile(testFileName, testSignalComboBox->currentIndex());
  }
}

void ProfilerDialog::profilerThreadFinished()
{
  delete profiler;

  msg->setProgressValue(100);
  msg->close();

  accept();
}

void ProfilerDialog::profilerProgressChanged(int progress)
{
  msg->setProgressValue(progress);
}
