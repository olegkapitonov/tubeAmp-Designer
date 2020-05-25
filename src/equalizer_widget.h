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

#ifndef EQUALIZERWIDGET_H
#define EQUALIZERWIDGET_H

#include <QtWidgets/QWidget>
#include <QPixmap>
#include <QPolygon>
#include <QString>
#include <QFont>
#include <QVector>

#include "freq_response_widget.h"

class EqualizerWidget : public QWidget
{
  Q_OBJECT

public:
  EqualizerWidget(QWidget *parent = nullptr);

  QVector<double> fLogValuesEq;
  QVector<double> dbValuesEq;

  QVector<double> fLogValuesFr;
  QVector<double> dbValuesFr;

  int maxDb;

  void drawBackground();
  void resetEq();

protected:
  QPoint mousePressPoint;
  double activeFLog;
  double activeDb;

  int activePoint;

  QRect backr;

  QFont infoFont;
  QString text;

  QPixmap *backbuffer;
  QPoint infop;
  QRect backinfor;

  int stepWidth;
  int stepHeight;
  int dbInStep;

  QRect margin;
  bool isResponseChanged = false;

  void paintEvent(QPaintEvent *);
  void mouseMoveEvent(QMouseEvent *);
  void mousePressEvent(QMouseEvent *);
  void mouseDoubleClickEvent(QMouseEvent *);
  void mouseReleaseEvent(QMouseEvent *);
  void resizeEvent(QResizeEvent *);

public slots:

signals:
  void responseChanged();
};

#endif // EQUALIZERWIDGET_H
