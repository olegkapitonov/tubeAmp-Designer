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

#ifndef NONLINEARWIDGET_H
#define NONLINEARWIDGET_H

#include <QtWidgets/QWidget>
#include <QPixmap>
#include <QPolygon>
#include <QString>
#include <QFont>

class NonlinearWidget : public QWidget
{
  Q_OBJECT

public:
  NonlinearWidget(QWidget *parent = nullptr);

  QVector<double> inValues;
  QVector<double> outValues;

  double maxIn;
  double maxOut;

  void drawBackground();

protected:
  QRect backr;

  QFont infoFont;
  QString text;

  QPixmap *backbuffer;
  QPoint infop;
  QRect backinfor;

  int stepWidth;
  int stepHeight;

  QRect margin;

  void paintEvent(QPaintEvent *);
  void resizeEvent(QResizeEvent *);
  void mouseMoveEvent(QMouseEvent *);

};

#endif //NONLINEARWIDGET_H
