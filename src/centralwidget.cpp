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

#include <QSizePolicy>
#include <QStyle>
#include <QApplication>

#include "centralwidget.h"

CentralWidget::CentralWidget(QWidget *parent, Processor *prc, Player *plr) :
  QWidget(parent)
{
  processor = prc;
  player = plr;
  QGridLayout *gbox = new QGridLayout(this);

  QWidget *moduleSelectBar = new QWidget(this);
  gbox->addWidget(moduleSelectBar, 0, 0, 1, 2);
  moduleSelectBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  QHBoxLayout *hbox = new QHBoxLayout(moduleSelectBar);

  preAmpFilterButton = new QPushButton(tr("PreAmp Filter"), moduleSelectBar);
  hbox->addWidget(preAmpFilterButton);
  preAmpFilterButton->setCheckable(true);
  preAmpFilterButton->setChecked(true);

  connect(preAmpFilterButton, &QPushButton::clicked, this,
          &CentralWidget::preAmpFilterButtonClicked);

  preAmpParamsButton = new QPushButton(tr("PreAmp Parameters"), moduleSelectBar);
  hbox->addWidget(preAmpParamsButton);
  preAmpParamsButton->setCheckable(true);

  connect(preAmpParamsButton, &QPushButton::clicked, this,
          &CentralWidget::preAmpParamsButtonClicked);

  toneStackButton = new QPushButton(tr("ToneStack"), moduleSelectBar);
  hbox->addWidget(toneStackButton);
  toneStackButton->setCheckable(true);

  connect(toneStackButton, &QPushButton::clicked, this, &CentralWidget::toneStackButtonClicked);

  powerAmpParamsButton = new QPushButton(tr("PowerAmp Parameters"), moduleSelectBar);
  hbox->addWidget(powerAmpParamsButton);
  powerAmpParamsButton->setCheckable(true);

  connect(powerAmpParamsButton, &QPushButton::clicked, this,
          &CentralWidget::powerAmpParamsButtonClicked);

  cabSymButton = new QPushButton(tr("Cabinet"), moduleSelectBar);
  hbox->addWidget(cabSymButton);
  cabSymButton->setCheckable(true);

  connect(cabSymButton, &QPushButton::clicked, this, &CentralWidget::cabSymButtonClicked);

  playerPanel = new PlayerPanel(moduleSelectBar, player, processor);
  hbox->addWidget(playerPanel);

  centralArea = new QScrollArea(this);

  preampFilterEditWidget = new PreampFilterEditWidget(this, processor);
  activeBlockEdit = preampFilterEditWidget;
  centralArea->setWidget(preampFilterEditWidget);

  gbox->addWidget(centralArea, 1, 1, 1, 1);

  tubeAmpPanel = new TubeAmpPanel(this, processor, player);
  gbox->addWidget(tubeAmpPanel, 1, 0, 1, 1);

  connect(tubeAmpPanel, &TubeAmpPanel::dialValueChanged, this, &CentralWidget::dialValueChanged);

  connect(player, &Player::peakRMSValueCalculated, tubeAmpPanel,
    &TubeAmpPanel::peakRMSValueChanged);

  centralArea->installEventFilter(this);
}

void CentralWidget::preAmpFilterButtonClicked()
{
  uncheckModuleSelectBar();
  preAmpFilterButton->setChecked(true);

  if (activeBlockEdit != preampFilterEditWidget)
  {
    preampFilterEditWidget = new PreampFilterEditWidget(this, processor);
    adjustWidget(preampFilterEditWidget);
    activeBlockEdit = preampFilterEditWidget;
    centralArea->setWidget(preampFilterEditWidget);
  }
}

void CentralWidget::preAmpParamsButtonClicked()
{
  uncheckModuleSelectBar();
  preAmpParamsButton->setChecked(true);

  if (activeBlockEdit != preampNonlinearEditWidget)
  {
    preampNonlinearEditWidget = new PreampNonlinearEditWidget(this, processor);
    adjustWidget(preampNonlinearEditWidget);
    activeBlockEdit = preampNonlinearEditWidget;
    centralArea->setWidget(preampNonlinearEditWidget);
  }
}

void CentralWidget::toneStackButtonClicked()
{
  uncheckModuleSelectBar();
  toneStackButton->setChecked(true);

  if (activeBlockEdit != tonestackEditWidget)
  {
    tonestackEditWidget = new TonestackEditWidget(this, processor);
    adjustWidget(tonestackEditWidget);
    activeBlockEdit = tonestackEditWidget;
    centralArea->setWidget(tonestackEditWidget);
  }
}

void CentralWidget::powerAmpParamsButtonClicked()
{
  uncheckModuleSelectBar();
  powerAmpParamsButton->setChecked(true);

  if (activeBlockEdit != ampNonlinearEditWidget)
  {
    ampNonlinearEditWidget = new AmpNonlinearEditWidget(this, processor);
    adjustWidget(ampNonlinearEditWidget);
    activeBlockEdit = ampNonlinearEditWidget;
    centralArea->setWidget(ampNonlinearEditWidget);
  }
}

void CentralWidget::cabSymButtonClicked()
{
  uncheckModuleSelectBar();
  cabSymButton->setChecked(true);

  if (activeBlockEdit != cabinetEditWidget)
  {
    cabinetEditWidget = new CabinetEditWidget(this, processor, player);
    adjustWidget(cabinetEditWidget);
    activeBlockEdit = cabinetEditWidget;
    centralArea->setWidget(cabinetEditWidget);
  }
}

void CentralWidget::uncheckModuleSelectBar()
{
  preAmpFilterButton->setChecked(false);
  preAmpParamsButton->setChecked(false);
  toneStackButton->setChecked(false);
  powerAmpParamsButton->setChecked(false);
  cabSymButton->setChecked(false);
}

bool CentralWidget::eventFilter(QObject *o, QEvent *e)
{
  if(o == centralArea && e->type() == QEvent::Resize)
  {
    adjustWidget(activeBlockEdit);
  }

  return false;
}

void CentralWidget::adjustWidget(QWidget *widget)
{
  if (widget != NULL)
  {
    widget->setMinimumWidth(centralArea->width()
        - qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent) - 5);
  }
}

void CentralWidget::dialValueChanged()
{
  if (activeBlockEdit == tonestackEditWidget)
  {
    activeBlockEdit->recalculate();
  }
}

void CentralWidget::reloadBlocks()
{
  tubeAmpPanel->resetControls();
  activeBlockEdit->recalculate();
  activeBlockEdit->resetControls();
}

void CentralWidget::updateBlocks()
{
  tubeAmpPanel->resetControls();
  activeBlockEdit->recalculate();
  activeBlockEdit->updateControls();
}
