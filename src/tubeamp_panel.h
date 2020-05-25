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

#ifndef TUBEAMPPANEL_H
#define TUBEAMPPANEL_H

#include <QFrame>
#include <QScrollArea>

#include "tadial.h"
#include "tameter.h"
#include "processor.h"
#include "player.h"

class TubeAmpPanel : public QFrame
{
  Q_OBJECT

public:
  TubeAmpPanel(QWidget *parent = nullptr, Processor *prc = nullptr,
    Player *plr = nullptr);

  void resetControls();

private:
  TADial *driveDial;
  TADial *bassDial;
  TADial *middleDial;
  TADial *trebleDial;
  TADial *volumeDial;
  TADial *levelDial;

  TAMeter *inputMeter;
  TAMeter *outputMeter;

  Processor *processor;
  Player *player;

  QWidget *scrollWidget;
  QScrollArea *scrollArea;

  void resizeEvent(QResizeEvent *);

public slots:
  void bassDialValueChanged(int newValue);
  void middleDialValueChanged(int newValue);
  void trebleDialValueChanged(int newValue);

  void driveDialValueChanged(int newValue);
  void volumeDialValueChanged(int newValue);
  void levelDialValueChanged(int newValue);

  void peakRMSValueChanged(float inputValue, float outputValue);

signals:
  void dialValueChanged();

};

#endif // TUBEAMPPANEL_H
