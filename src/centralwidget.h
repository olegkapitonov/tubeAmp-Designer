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

#ifndef CENTRALWIDGET_H
#define CENTRALWIDGET_H

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QEvent>

#include <QtWidgets/QWidget>

#include "block_edit_widget.h"
#include "preamp_filter_edit_widget.h"
#include "preamp_nonlinear_edit_widget.h"
#include "tonestack_edit_widget.h"
#include "amp_nonlinear_edit_widget.h"
#include "cabinet_edit_widget.h"
#include "player_panel.h"
#include "tubeamp_panel.h"
#include "processor.h"
#include "player.h"

class CentralWidget : public QWidget
{
  Q_OBJECT

public:
  CentralWidget(QWidget *parent, Processor *prc, Player *plr);

  PlayerPanel *playerPanel;

  void reloadBlocks();
  void updateBlocks();

private:
  QPushButton *preAmpFilterButton;
  QPushButton *preAmpParamsButton;
  QPushButton *toneStackButton;
  QPushButton *powerAmpParamsButton;
  QPushButton *cabSymButton;
  QScrollArea *centralArea;

  BlockEditWidget *activeBlockEdit;

  PreampFilterEditWidget *preampFilterEditWidget;
  PreampNonlinearEditWidget *preampNonlinearEditWidget;
  TonestackEditWidget *tonestackEditWidget;
  AmpNonlinearEditWidget *ampNonlinearEditWidget;
  CabinetEditWidget *cabinetEditWidget;
  TubeAmpPanel *tubeAmpPanel;

  Processor *processor;
  Player *player;

  void uncheckModuleSelectBar();
  bool eventFilter(QObject *o, QEvent *e);
  void adjustWidget(QWidget *widget);

public slots:
  void preAmpFilterButtonClicked();
  void preAmpParamsButtonClicked();
  void toneStackButtonClicked();
  void powerAmpParamsButtonClicked();
  void cabSymButtonClicked();

  void dialValueChanged();
};

#endif // CENTRALWIDGET_H
