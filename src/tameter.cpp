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

#include <QPainter>
#include <cmath>

#include "tameter.h"

TAMeter::TAMeter(QWidget *parent) : QWidget(parent)
{
  value = -60.0;
}

void TAMeter::paintEvent(QPaintEvent *)
{
  QPainter painter(this);

  painter.fillRect(0, 0, width(), height(), QBrush(QColor(0, 0, 0)));

  if (value < -60.0)
  {
    value = -60.0;
  }

  if (value > 0.0)
  {
    value = 0.0;
  }

  int barWidth = width() * (1.0 - (value / (-60.0)));

  QLinearGradient barGrad(QPointF(0, 0), QPointF(width(), 0));
  barGrad.setColorAt(0, QColor(0, 50, 0));
  barGrad.setColorAt((1.0 - (-25.0 / (-60.0))), QColor(100, 150, 0));
  barGrad.setColorAt((1.0 - (-20.0 / (-60.0))), QColor(200, 100, 0));
  barGrad.setColorAt(1, QColor(255, 0, 0));

  painter.fillRect(0, 0, barWidth, height(), barGrad);
}

void TAMeter::setValue(float v)
{
  value = v;

  if (value > 0.0) value = 0.0;
  if (value < -60.0) value = -60.0;

  repaint(0,0,-1,-1);
}
