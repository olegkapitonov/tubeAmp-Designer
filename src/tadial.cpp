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
#include <math.h>

#include "tadial.h"

TADial::TADial(QWidget *parent) : QWidget(parent)
{
  dialSize = 80;
  dialLeft = width() / 2 - dialSize / 2;
  dialTop = 0;
  dialMaxAngle = 140;
  setMinimumWidth(85);
  value = 0;
  startValue = 0;
}

void TADial::paintEvent(QPaintEvent *)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  painter.setBrush(QBrush(QColor(200,200,200)));
  painter.drawEllipse(dialLeft + 10, dialTop + 10, dialSize - 20, dialSize - 20);

  painter.setBrush(QBrush(QColor(0,0,0)));
  painter.drawEllipse(dialLeft + 20, dialTop + 20, dialSize - 40, dialSize - 40);

  int pointerAngle = value/100.0*(dialMaxAngle * 2.0) - dialMaxAngle;

  painter.translate(width() / 2, height() / 2);
  painter.rotate(pointerAngle);
  painter.setPen(QPen(QColor(0,0,0)));
  painter.setBrush(QBrush(QColor(255,255,255)));

  painter.drawRect(-5,-dialSize / 2,10,20);

  painter.setPen(QPen(QColor(2.54*value,127*(1.0-value/100.0),
                               (1.0-2.0*fabs(0.5-value/100.0))*254)));
  painter.drawLine(-5,-dialSize / 2 + 7,5,-dialSize / 2 + 7);
  painter.drawLine(-5,-dialSize / 2 + 10,5,-dialSize / 2 + 10);
  painter.drawLine(-5,-dialSize / 2 + 13,5,-dialSize / 2 + 13);
}

void TADial::mouseMoveEvent(QMouseEvent *event)
{
  QPoint pos = event->pos();
  value = startValue + startScroll.y()-pos.y();

  if (value<0)
  {
    value = 0;
  }
  else if (value>100)
  {
    value = 100;
  }

  repaint(0,0,-1,-1);

  emit valueChanged(value);
}

void TADial::mousePressEvent(QMouseEvent *event)
{
  startScroll = event->pos();
  startValue = value;
}

void TADial::resizeEvent(QResizeEvent *)
{
  dialLeft = width() / 2 - dialSize / 2;
  dialTop = height() / 2 - dialSize / 2;
}

void TADial::setValue(int v)
{
  value = v;

  if (value > 100) value = 100;
  if (value < 0) value = 0;

  repaint(0,0,-1,-1);
}
