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

#include <QHBoxLayout>
#include <cmath>

#include "slide_box_widget.h"

SlideBoxWidget::SlideBoxWidget(const QString &text, float min, float max, QWidget *parent)
 : QWidget(parent)
{
  minValue = min;
  maxValue = max;

  QHBoxLayout *lay = new QHBoxLayout(this);

  QLabel *label = new QLabel(text,this);
  lay->addWidget(label);

  label->setMinimumWidth(200);

  slider = new QSlider(Qt::Horizontal, this);
  lay->addWidget(slider);
  slider->setRange(0, 1000000);
  slider->setSingleStep(1);

  valueLabel = new QLabel("0", this);
  lay->addWidget(valueLabel);

  valueLabel->setMinimumWidth(100);

  QString strValue;
  strValue = QString("%1").arg(minValue, 4, 'f', 2);
  valueLabel->setText(strValue);

  connect(slider, &QSlider::valueChanged, this, &SlideBoxWidget::sliderValueChanged);
}

void SlideBoxWidget::sliderValueChanged(int value)
{
  QString strValue;
  float newValue = round(100.0 * ((double)value /
    1000000.0 * (maxValue - minValue) + minValue)) / 100.0;

  strValue = QString("%1").arg(newValue, 4, 'f', 2);
  valueLabel->setText(strValue);

  emit valueChanged(newValue);
}

void SlideBoxWidget::setValue(float value)
{
  slider->setValue((value - minValue) / (maxValue - minValue) * 1000000.0);
}
