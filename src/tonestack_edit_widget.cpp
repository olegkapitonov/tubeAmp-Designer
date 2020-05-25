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

#include <QVBoxLayout>

#include <cmath>
#include <gsl/gsl_complex.h>
#include <gsl/gsl_complex_math.h>

#include "tonestack_edit_widget.h"

float db2gain(float db)
{
  return pow(10.0, db / 20.0);
}

TonestackEditWidget::TonestackEditWidget(QWidget *parent, Processor *prc) :
  BlockEditWidget(parent)
{
  processor = prc;
  setFrameShape(QFrame::Panel);

  QVBoxLayout *vbox = new QVBoxLayout(this);

  QLabel *tonestackLabel = new QLabel(tr("Tonestack Frequency Response"), this);
  vbox->addWidget(tonestackLabel);
  tonestackLabel->setMaximumHeight(30);
  tonestackLabel->setAlignment(Qt::AlignHCenter);
  QFont tonestackLabelFont = tonestackLabel->font();
  tonestackLabelFont.setPointSize(15);
  tonestackLabel->setFont(tonestackLabelFont);

  vbox->addWidget(tonestackLabel);

  freqResponse = new FreqResponseWidget(this);
  vbox->addWidget(freqResponse);

  freqResponse->maxDb = 20.0;

  bassFreqSlide = new SlideBoxWidget(tr("Bass Frequency"), 10.0, 500.0, this);
  vbox->addWidget(bassFreqSlide);

  connect(bassFreqSlide, &SlideBoxWidget::valueChanged, this,
          &TonestackEditWidget::bassFreqChanged);

  bassBandSlide = new SlideBoxWidget(tr("Bass Bandwidth"), 100.0, 1000.0, this);
  vbox->addWidget(bassBandSlide);

  connect(bassBandSlide, &SlideBoxWidget::valueChanged, this,
          &TonestackEditWidget::bassBandChanged);

  middleFreqSlide = new SlideBoxWidget(tr("Middle Frequency"), 100.0, 1000.0, this);
  vbox->addWidget(middleFreqSlide);

  connect(middleFreqSlide, &SlideBoxWidget::valueChanged, this,
          &TonestackEditWidget::middleFreqChanged);

  middleBandSlide = new SlideBoxWidget(tr("Middle Bandwidth"), 200.0, 5000.0, this);
  vbox->addWidget(middleBandSlide);

  connect(middleBandSlide, &SlideBoxWidget::valueChanged, this, &TonestackEditWidget::middleBandChanged);

  trebleFreqSlide = new SlideBoxWidget(tr("Treble Frequency"), 2000.0, 20000.0, this);
  vbox->addWidget(trebleFreqSlide);

  connect(trebleFreqSlide, &SlideBoxWidget::valueChanged, this,
          &TonestackEditWidget::trebleFreqChanged);

  trebleBandSlide = new SlideBoxWidget(tr("Treble Bandwidth"), 3000.0, 40000.0, this);
  vbox->addWidget(trebleBandSlide);

  connect(trebleBandSlide, &SlideBoxWidget::valueChanged, this,
          &TonestackEditWidget::trebleBandChanged);

  recalculate();
  resetControls();
}

void TonestackEditWidget::recalculate()
{
  st_profile profile = processor->getProfile();
  stControls controls = processor->getControls();

  QVector<float> freqs(100);
  for (int i = 0; i < freqs.size(); i++)
  {
    float fLog = (log10(20000.0) - log10(10.0))*(float)(i) / 99.0 + log10(10.0);
    freqs[i] = pow(10, fLog);
  }

  freqResponse->fLogValues.resize(freqs.size());
  freqResponse->dbValues.resize(freqs.size());

  for (int i = 0; i < freqs.size(); i++)
  {
    freqResponse->fLogValues[i] = log10(freqs[i]);

    gsl_complex j_w_norm = gsl_complex_rect(0, freqs[i] / profile.tonestack_low_freq);
    gsl_complex B_norm = gsl_complex_rect(profile.tonestack_low_band /
      profile.tonestack_low_freq, 0);
    gsl_complex gain = gsl_complex_rect(db2gain(controls.low), 0);

    gsl_complex A_numer = gsl_complex_add(
      gsl_complex_add(
        gsl_complex_mul(j_w_norm, j_w_norm),
        gsl_complex_mul(gsl_complex_mul(B_norm, gain), j_w_norm)
      ),
      gsl_complex_rect(1.0, 0.0)
    );

    gsl_complex A_denom = gsl_complex_add(
      gsl_complex_add(
        gsl_complex_mul(j_w_norm, j_w_norm),
        gsl_complex_mul(B_norm, j_w_norm)
      ),
      gsl_complex_rect(1.0, 0.0)
    );

    gsl_complex A_low = gsl_complex_div(A_numer, A_denom);


    j_w_norm = gsl_complex_rect(0, freqs[i] / profile.tonestack_middle_freq);
    B_norm = gsl_complex_rect(profile.tonestack_middle_band / profile.tonestack_middle_freq, 0);
    gain = gsl_complex_rect(db2gain(controls.middle), 0);

    A_numer = gsl_complex_add(
      gsl_complex_add(
        gsl_complex_mul(j_w_norm, j_w_norm),
        gsl_complex_mul(gsl_complex_mul(B_norm, gain), j_w_norm)
      ),
      gsl_complex_rect(1.0, 0.0)
    );

    A_denom = gsl_complex_add(
      gsl_complex_add(
        gsl_complex_mul(j_w_norm, j_w_norm),
        gsl_complex_mul(B_norm, j_w_norm)
      ),
      gsl_complex_rect(1.0, 0.0)
    );

    gsl_complex A_middle = gsl_complex_div(A_numer, A_denom);


    j_w_norm = gsl_complex_rect(0, freqs[i] / profile.tonestack_high_freq);
    B_norm = gsl_complex_rect(profile.tonestack_high_band / profile.tonestack_high_freq, 0);
    gain = gsl_complex_rect(db2gain(controls.high), 0);

    A_numer = gsl_complex_add(
      gsl_complex_add(
        gsl_complex_mul(j_w_norm, j_w_norm),
        gsl_complex_mul(gsl_complex_mul(B_norm, gain), j_w_norm)
      ),
      gsl_complex_rect(1.0, 0.0)
    );

    A_denom = gsl_complex_add(
      gsl_complex_add(
        gsl_complex_mul(j_w_norm, j_w_norm),
        gsl_complex_mul(B_norm, j_w_norm)
      ),
      gsl_complex_rect(1.0, 0.0)
    );

    gsl_complex A_treble = gsl_complex_div(A_numer, A_denom);


    float A = gsl_complex_abs(A_low) * gsl_complex_abs(A_middle) * gsl_complex_abs(A_treble);

    freqResponse->dbValues[i] = 20.0 * log10(A);
  }
  freqResponse->update(0,0,width(),height());
}

void TonestackEditWidget::bassFreqChanged(float value)
{
  st_profile profile = processor->getProfile();
  profile.tonestack_low_freq = value;
  processor->setProfile(profile);

  recalculate();
}

void TonestackEditWidget::middleFreqChanged(float value)
{
  st_profile profile = processor->getProfile();
  profile.tonestack_middle_freq = value;
  processor->setProfile(profile);

  recalculate();
}

void TonestackEditWidget::trebleFreqChanged(float value)
{
  st_profile profile = processor->getProfile();
  profile.tonestack_high_freq = value;
  processor->setProfile(profile);

  recalculate();
}

void TonestackEditWidget::bassBandChanged(float value)
{
  st_profile profile = processor->getProfile();
  profile.tonestack_low_band = value;
  processor->setProfile(profile);

  recalculate();
}

void TonestackEditWidget::middleBandChanged(float value)
{
  st_profile profile = processor->getProfile();
  profile.tonestack_middle_band = value;
  processor->setProfile(profile);

  recalculate();
}

void TonestackEditWidget::trebleBandChanged(float value)
{
  st_profile profile = processor->getProfile();
  profile.tonestack_high_band = value;
  processor->setProfile(profile);

  recalculate();
}

void TonestackEditWidget::resetControls()
{
  st_profile profile = processor->getProfile();

  bassFreqSlide->setValue(profile.tonestack_low_freq);
  bassBandSlide->setValue(profile.tonestack_low_band);
  middleBandSlide->setValue(profile.tonestack_middle_band);
  middleFreqSlide->setValue(profile.tonestack_middle_freq);
  trebleBandSlide->setValue(profile.tonestack_high_band);
  trebleFreqSlide->setValue(profile.tonestack_high_freq);
}
