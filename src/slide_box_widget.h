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

#ifndef SLIDEBOXWIDGET_H
#define SLIDEBOXWIDGET_H

#include <QtWidgets/QWidget>
#include <QSlider>
#include <QLabel>

class SlideBoxWidget : public QWidget
{
  Q_OBJECT

public:
  SlideBoxWidget(const QString &text, float min, float max, QWidget *parent = nullptr);

  float minValue;
  float maxValue;

  void setValue(float value);

private:
  QLabel *valueLabel;
  QSlider *slider;

protected slots:
  void sliderValueChanged(int value);

signals:
  void valueChanged(float value);

};

#endif //SLIDEBOXWIDGET_H
