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
#include <QSlider>

#include <cmath>

#include "preamp_nonlinear_edit_widget.h"
#include "freq_response_widget.h"

PreampNonlinearEditWidget::PreampNonlinearEditWidget(QWidget *parent, Processor *prc) :
  BlockEditWidget(parent)
{
  processor = prc;
  setFrameShape(QFrame::Panel);

  QVBoxLayout *vbox = new QVBoxLayout(this);

  QLabel *preampNonlinearLabel = new QLabel(tr("Preamp Nonlinear Function"), this);
  vbox->addWidget(preampNonlinearLabel);
  preampNonlinearLabel->setMaximumHeight(30);
  preampNonlinearLabel->setAlignment(Qt::AlignHCenter);
  QFont preampNonlinearLabelFont = preampNonlinearLabel->font();
  preampNonlinearLabelFont.setPointSize(15);
  preampNonlinearLabel->setFont(preampNonlinearLabelFont);

  vbox->addWidget(preampNonlinearLabel);

  nonlinear = new NonlinearWidget(this);
  vbox->addWidget(nonlinear);

  biasSlide = new SlideBoxWidget(tr("Bias"), -1.0, 1.0, this);
  vbox->addWidget(biasSlide);

  connect(biasSlide, &SlideBoxWidget::valueChanged, this,
          &PreampNonlinearEditWidget::biasChanged);

  uporSlide = new SlideBoxWidget(tr("Saturation Level"), 0.1, 2.0, this);
  vbox->addWidget(uporSlide);

  connect(uporSlide, &SlideBoxWidget::valueChanged, this,
          &PreampNonlinearEditWidget::uporChanged);

  kregSlide = new SlideBoxWidget(tr("Saturation Hard/Soft"), 0.0, 10.0, this);
  vbox->addWidget(kregSlide);

  connect(kregSlide, &SlideBoxWidget::valueChanged, this,
          &PreampNonlinearEditWidget::kregChanged);

  levelSlide = new SlideBoxWidget(tr("Gain Level"), -46.0, 10.0, this);
  vbox->addWidget(levelSlide);

  connect(levelSlide, &SlideBoxWidget::valueChanged, this,
          &PreampNonlinearEditWidget::levelChanged);

  resetControls();
  recalculate();
}

void PreampNonlinearEditWidget::recalculate()
{
  st_profile profile = processor->getProfile();
  nonlinear->inValues.resize(1000);
  nonlinear->outValues.resize(1000);

  for (int i = 0; i < nonlinear->inValues.size(); i++)
  {
    nonlinear->inValues[i] = -3.0 + (float)i / (nonlinear->inValues.size() - 1) * 6.0;
    nonlinear->outValues[i] = processor->tube(nonlinear->inValues[i],
                                              profile.preamp_Kreg,
                                              profile.preamp_Upor,
                                              profile.preamp_bias,
                                              -profile.preamp_Upor);
  }

  nonlinear->maxIn = 3.0;
  nonlinear->maxOut = 2.0;
  nonlinear->drawBackground();
  nonlinear->update(0,0,width(),height());
}

void PreampNonlinearEditWidget::biasChanged(float value)
{
  st_profile profile = processor->getProfile();
  profile.preamp_bias = value;
  processor->setProfile(profile);

  recalculate();
}

void PreampNonlinearEditWidget::uporChanged(float value)
{
  st_profile profile = processor->getProfile();
  profile.preamp_Upor = value;
  processor->setProfile(profile);

  recalculate();
}

void PreampNonlinearEditWidget::kregChanged(float value)
{
  st_profile profile = processor->getProfile();
  profile.preamp_Kreg = value;
  processor->setProfile(profile);

  recalculate();
}

void PreampNonlinearEditWidget::levelChanged(float value)
{
  st_profile profile = processor->getProfile();
  profile.preamp_level = pow(10.0, value / 20.0);
  processor->setProfile(profile);

  recalculate();
}

void PreampNonlinearEditWidget::resetControls()
{
  st_profile profile = processor->getProfile();

  biasSlide->setValue(profile.preamp_bias);
  levelSlide->setValue(20.0 * log10(profile.preamp_level));
  kregSlide->setValue(profile.preamp_Kreg);
  uporSlide->setValue(profile.preamp_Upor);
}

void PreampNonlinearEditWidget::updateControls()
{
  resetControls();
}
