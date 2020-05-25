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

#include <QLabel>
#include <QGridLayout>
#include <cmath>

#include "tubeamp_panel.h"

TubeAmpPanel::TubeAmpPanel(QWidget *parent, Processor *prc, Player *plr) :
  QFrame(parent)
{
  processor = prc;
  player = plr;
  setFrameShape(QFrame::Panel);
  setMinimumWidth(200);
  setMaximumWidth(200);

  scrollArea = new QScrollArea(this);
  QHBoxLayout *mainLayout = new QHBoxLayout(this);

  scrollWidget = new QWidget(this);
  mainLayout->addWidget(scrollWidget);
  scrollWidget->setMinimumHeight(550);
  scrollWidget->setMinimumWidth(200);
  scrollWidget->setMaximumWidth(200);
  scrollArea->setWidget(scrollWidget);

  QGridLayout *gbox = new QGridLayout(scrollWidget);
  gbox->setAlignment(Qt::AlignHCenter);
  QLabel *titleLabel = new QLabel(tr("tubeAmp Controls"), scrollWidget);
  gbox->addWidget(titleLabel, 0, 0, 1, 2);
  titleLabel->setMaximumHeight(30);
  titleLabel->setAlignment(Qt::AlignHCenter);
  QFont titleLabelFont = titleLabel->font();
  titleLabelFont.setPointSize(15);
  titleLabel->setFont(titleLabelFont);

  QLabel *driveLabel = new QLabel(tr("Drive"), scrollWidget);
  driveLabel->setAlignment(Qt::AlignHCenter);
  driveLabel->setMaximumHeight(30);
  gbox->addWidget(driveLabel , 1, 0, 1, 1);

  driveDial = new TADial(scrollWidget);
  gbox->addWidget(driveDial, 2, 0, 1, 1);

  connect(driveDial, &TADial::valueChanged, this, &TubeAmpPanel::driveDialValueChanged);

  QLabel *bassLabel = new QLabel(tr("Bass"), scrollWidget);
  bassLabel->setAlignment(Qt::AlignHCenter);
  bassLabel->setMaximumHeight(30);
  gbox->addWidget(bassLabel , 1, 1, 1, 1);

  bassDial = new TADial(scrollWidget);
  gbox->addWidget(bassDial, 2, 1, 1, 1);

  connect(bassDial, &TADial::valueChanged, this, &TubeAmpPanel::bassDialValueChanged);

  QLabel *middleLabel = new QLabel(tr("Middle"), scrollWidget);
  middleLabel->setAlignment(Qt::AlignHCenter);
  middleLabel->setMaximumHeight(30);
  gbox->addWidget(middleLabel , 3, 0, 1, 1);

  middleDial = new TADial(scrollWidget);
  gbox->addWidget(middleDial, 4, 0, 1, 1);

  connect(middleDial, &TADial::valueChanged, this, &TubeAmpPanel::middleDialValueChanged);

  QLabel *trebleLabel = new QLabel(tr("Treble"), scrollWidget);
  trebleLabel->setAlignment(Qt::AlignHCenter);
  trebleLabel->setMaximumHeight(30);
  gbox->addWidget(trebleLabel , 3, 1, 1, 1);

  trebleDial = new TADial(scrollWidget);
  gbox->addWidget(trebleDial, 4, 1, 1, 1);

  connect(trebleDial, &TADial::valueChanged, this, &TubeAmpPanel::trebleDialValueChanged);

  QLabel *volumeLabel = new QLabel(tr("Volume"), scrollWidget);
  volumeLabel->setAlignment(Qt::AlignHCenter);
  volumeLabel->setMaximumHeight(30);
  gbox->addWidget(volumeLabel , 5, 0, 1, 1);

  volumeDial = new TADial(scrollWidget);
  gbox->addWidget(volumeDial, 6, 0, 1, 1);

  connect(volumeDial, &TADial::valueChanged, this, &TubeAmpPanel::volumeDialValueChanged);

  QLabel *levelLabel = new QLabel(tr("Level"), scrollWidget);
  levelLabel->setAlignment(Qt::AlignHCenter);
  levelLabel->setMaximumHeight(30);
  gbox->addWidget(levelLabel , 5, 1, 1, 1);

  levelDial = new TADial(scrollWidget);
  gbox->addWidget(levelDial, 6, 1, 1, 1);

  connect(levelDial, &TADial::valueChanged, this, &TubeAmpPanel::levelDialValueChanged);

  QLabel *inputMeterLabel = new QLabel(tr("Input Level"), scrollWidget);
  inputMeterLabel->setMaximumHeight(24);
  gbox->addWidget(inputMeterLabel, 7, 0, 1, 2);

  inputMeter = new TAMeter(scrollWidget);
  inputMeter->setMaximumHeight(24);
  gbox->addWidget(inputMeter, 8, 0, 1, 2);

  QLabel *outputMeterLabel = new QLabel(tr("Output Level"), scrollWidget);
  outputMeterLabel->setMaximumHeight(24);
  gbox->addWidget(outputMeterLabel, 9, 0, 1, 2);

  outputMeter = new TAMeter(scrollWidget);
  outputMeter->setMaximumHeight(24);
  gbox->addWidget(outputMeter, 10, 0, 1, 2);

  resetControls();
}

void TubeAmpPanel::resizeEvent(QResizeEvent *)
{
  scrollArea->setMinimumWidth(width());
  scrollArea->setMinimumHeight(height());
}

void TubeAmpPanel::bassDialValueChanged(int newValue)
{
  stControls controls = processor->getControls();
  controls.low = (newValue - 50) / 5.0;
  processor->setControls(controls);

  emit dialValueChanged();
}

void TubeAmpPanel::middleDialValueChanged(int newValue)
{
  stControls controls = processor->getControls();
  controls.middle = (newValue - 50) / 5.0;
  processor->setControls(controls);

  emit dialValueChanged();
}

void TubeAmpPanel::trebleDialValueChanged(int newValue)
{
  stControls controls = processor->getControls();
  controls.high = (newValue - 50) / 5.0;
  processor->setControls(controls);

  emit dialValueChanged();
}

void TubeAmpPanel::driveDialValueChanged(int newValue)
{
  stControls controls = processor->getControls();
  controls.drive = newValue;
  processor->setControls(controls);

  emit dialValueChanged();
}

void TubeAmpPanel::volumeDialValueChanged(int newValue)
{
  stControls controls = processor->getControls();
  controls.mastergain = newValue;
  processor->setControls(controls);

  emit dialValueChanged();
}

void TubeAmpPanel::levelDialValueChanged(int newValue)
{
  stControls controls = processor->getControls();
  controls.volume = (float)newValue / 100.0;
  processor->setControls(controls);

  player->setLevel((float)newValue / 100.0);

  emit dialValueChanged();
}

void TubeAmpPanel::resetControls()
{
  stControls ctrls = processor->getControls();
  driveDial->setValue(ctrls.drive);
  bassDial->setValue((ctrls.low + 10.0) * 5.0);
  middleDial->setValue((ctrls.middle + 10.0) * 5.0);
  trebleDial->setValue((ctrls.high + 10.0) * 5.0);
  volumeDial->setValue(ctrls.mastergain);
  levelDial->setValue(ctrls.volume * 100.0);
}

void TubeAmpPanel::peakRMSValueChanged(float inputValue, float outputValue)
{
  float dbInputValue = 20.0 * log10(inputValue);
  float dbOutputValue = 20.0 * log10(outputValue);
  inputMeter->setValue(dbInputValue);
  outputMeter->setValue(dbOutputValue);
}
