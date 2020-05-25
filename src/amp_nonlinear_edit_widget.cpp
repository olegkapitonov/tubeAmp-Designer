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

#include "amp_nonlinear_edit_widget.h"
#include "freq_response_widget.h"

AmpNonlinearEditWidget::AmpNonlinearEditWidget(QWidget *parent, Processor *prc) :
  BlockEditWidget(parent)
{
  processor = prc;
  setFrameShape(QFrame::Panel);

  QVBoxLayout *vbox = new QVBoxLayout(this);

  QLabel *ampNonlinearLabel = new QLabel(tr("Power Amp Nonlinear Function"), this);
  vbox->addWidget(ampNonlinearLabel);
  ampNonlinearLabel->setMaximumHeight(30);
  ampNonlinearLabel->setAlignment(Qt::AlignHCenter);
  QFont ampNonlinearLabelFont = ampNonlinearLabel->font();
  ampNonlinearLabelFont.setPointSize(15);
  ampNonlinearLabel->setFont(ampNonlinearLabelFont);

  vbox->addWidget(ampNonlinearLabel);

  nonlinear = new NonlinearWidget(this);
  vbox->addWidget(nonlinear);

  biasSlide = new SlideBoxWidget(tr("Bias"), 0.0, 1.0, this);
  vbox->addWidget(biasSlide);

  connect(biasSlide, &SlideBoxWidget::valueChanged, this, &AmpNonlinearEditWidget::biasChanged);

  uporSlide = new SlideBoxWidget(tr("Saturation Level"), 0.0, 10.0, this);
  vbox->addWidget(uporSlide);

  connect(uporSlide, &SlideBoxWidget::valueChanged, this, &AmpNonlinearEditWidget::uporChanged);

  kregSlide = new SlideBoxWidget(tr("Saturation Hard/Soft"), 0.0, 10.0, this);
  vbox->addWidget(kregSlide);

  connect(kregSlide, &SlideBoxWidget::valueChanged, this, &AmpNonlinearEditWidget::kregChanged);

  levelSlide = new SlideBoxWidget(tr("Gain Level"), -20.0, 20.0, this);
  vbox->addWidget(levelSlide);

  connect(levelSlide, &SlideBoxWidget::valueChanged, this, &AmpNonlinearEditWidget::levelChanged);

  sagCoeffSlide = new SlideBoxWidget(tr("Voltage Sag Strength"), 0.0, 10.0, this);
  vbox->addWidget(sagCoeffSlide);

  connect(sagCoeffSlide, &SlideBoxWidget::valueChanged, this,
          &AmpNonlinearEditWidget::sagCoeffChanged);

  sagTimeSlide = new SlideBoxWidget(tr("Voltage Sag Time"), 0.0, 0.5, this);
  vbox->addWidget(sagTimeSlide);

  connect(sagTimeSlide, &SlideBoxWidget::valueChanged, this,
          &AmpNonlinearEditWidget::sagTimeChanged);

  masterOutputLevelSlide = new SlideBoxWidget(tr("MASTER OUTPUT LEVEL"), 0.0, 2.0, this);
  vbox->addWidget(masterOutputLevelSlide);

  connect(masterOutputLevelSlide, &SlideBoxWidget::valueChanged, this,
          &AmpNonlinearEditWidget::masterOutputLevelChanged);

  recalculate();
  resetControls();
}

void AmpNonlinearEditWidget::recalculate()
{
  st_profile profile = processor->getProfile();

  nonlinear->inValues.resize(1000);
  nonlinear->outValues.resize(1000);

  for (int i = 0; i < nonlinear->inValues.size(); i++)
  {
    nonlinear->inValues[i] = -3.0 + (float)i / (nonlinear->inValues.size() - 1) * 6.0;
    nonlinear->outValues[i] = (processor->tube(nonlinear->inValues[i], profile.amp_Kreg,
      profile.amp_Upor, profile.amp_bias, 0.0)
      - processor->tube(-nonlinear->inValues[i], profile.amp_Kreg,
      profile.amp_Upor, profile.amp_bias, 0.0));
  }

  nonlinear->maxIn = 3.0;
  nonlinear->maxOut = 2.0;

  nonlinear->drawBackground();
  nonlinear->update(0,0,width(),height());
}

void AmpNonlinearEditWidget::biasChanged(float value)
{
  st_profile profile = processor->getProfile();
  profile.amp_bias = value;
  processor->setProfile(profile);

  recalculate();
}

void AmpNonlinearEditWidget::uporChanged(float value)
{
  st_profile profile = processor->getProfile();
  profile.amp_Upor = value;
  processor->setProfile(profile);

  recalculate();
}

void AmpNonlinearEditWidget::kregChanged(float value)
{
  st_profile profile = processor->getProfile();
  profile.amp_Kreg = value;
  processor->setProfile(profile);

  recalculate();
}

void AmpNonlinearEditWidget::levelChanged(float value)
{
  st_profile profile = processor->getProfile();
  profile.amp_level = pow(10.0, value / 20.0);
  processor->setProfile(profile);

  recalculate();
}

void AmpNonlinearEditWidget::sagCoeffChanged(float value)
{
  st_profile profile = processor->getProfile();
  profile.sag_coeff = value;
  processor->setProfile(profile);

  recalculate();
}

void AmpNonlinearEditWidget::sagTimeChanged(float value)
{
  st_profile profile = processor->getProfile();
  profile.sag_time = value;
  processor->setProfile(profile);

  recalculate();
}

void AmpNonlinearEditWidget::resetControls()
{
  st_profile profile = processor->getProfile();

  biasSlide->setValue(profile.amp_bias);
  uporSlide->setValue(profile.amp_Upor);
  sagTimeSlide->setValue(profile.sag_time);
  sagCoeffSlide->setValue(profile.sag_coeff);
  levelSlide->setValue(20.0 * log10(profile.amp_level));
  kregSlide->setValue(profile.amp_Kreg);
  masterOutputLevelSlide->setValue(profile.output_level);
}

void AmpNonlinearEditWidget::updateControls()
{
  resetControls();
}

void AmpNonlinearEditWidget::masterOutputLevelChanged(float value)
{
  st_profile profile = processor->getProfile();
  profile.output_level = value;
  processor->setProfile(profile);
}
