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
#include <QPaintEvent>
#include <QPolygon>

#include <gsl/gsl_spline.h>

#include "nonlinear_widget.h"

NonlinearWidget::NonlinearWidget(QWidget *parent) :
  QWidget(parent)
{
  QPalette Pal(palette());
  Pal.setColor(QPalette::Background, Qt::black);
  setAutoFillBackground(true);
  setPalette(Pal);

  setMaximumHeight(400);
  setMinimumHeight(400);

  backbuffer = new QPixmap(100, 100);
  setMouseTracking(true);

  margin.setLeft(60);
  margin.setRight(20);
  margin.setTop(40);
  margin.setBottom(50);

  maxIn = 3.0;
  maxOut = 2.0;

  for (int i = 0; i < 12; i++)
  {
    inValues.append(i * 2.0 / 10.0 - 1.0);
    outValues.append(i * 2.0 / 10.0 - 1.0);
  }
}

void NonlinearWidget::paintEvent(QPaintEvent * event)
{
  QPainter qp(this);

  for (QRegion::const_iterator rects = event->region().begin();
       rects!= event->region().end(); rects++)
  {
    QRect r = *rects;
    qp.drawPixmap(r,*backbuffer,r);
  }

  gsl_interp_accel *acc = gsl_interp_accel_alloc ();
  gsl_spline *spline = gsl_spline_alloc (gsl_interp_cspline, inValues.count());

  gsl_spline_init (spline, inValues.data(), outValues.data(), inValues.count());

  int graphWidth = width() - margin.left() - margin.right();

  QPolygon polyline1(graphWidth);

  for (int i=0; i < graphWidth; i++)
  {
    double interpolatedIn = (i - graphWidth / 2.0) / (graphWidth / 2.0) * maxIn;
    double interpolatedOut = gsl_spline_eval(spline, interpolatedIn, acc);

    int x = (1.0 + interpolatedIn / maxIn) * stepWidth * 20.0 / 2.0;
    int y = (1.0 - interpolatedOut / maxOut) * stepHeight * 20.0 / 2.0;

    if (y > (height() - margin.bottom() - margin.top()))
    {
      y = height() - margin.bottom() - margin.top();
    }

    if (y < 0)
    {
      y = 0;
    }

    polyline1.setPoint(i, x + margin.left(), y + margin.top());
  }

  gsl_spline_free(spline);
  gsl_interp_accel_free(acc);

  qp.setPen(QPen(QColor(0,255,0)));
  qp.drawPolyline(polyline1);

  qp.setPen(QPen(QColor(255,255,255)));
  infoFont = QFont("Sans",12);
  qp.setFont(infoFont);

  qp.drawText(infop,text);
}


void NonlinearWidget::drawBackground()
{
  QPainter qp(backbuffer);
  qp.fillRect(0,0,width(),height(),QBrush(QColor(0,0,0)));

  QPen penThin(QColor(80,80,80));
  QPen penBold(QColor(150,150,150),1);
  QPen penZero(QColor(150,150,150),3);
  QPen penText(QColor(255,255,255));

  QFontMetrics fm(QFont("Sans",10));
  qp.setFont(QFont("Sans",10));
  qp.setPen(penText);
  QString inputLevelString = tr("Input Level");

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
  int pixelsWide = fm.horizontalAdvance(inputLevelString);
#else
  int pixelsWide = fm.boundingRect(inputLevelString).width();
#endif

  qp.drawText(width() / 2 -pixelsWide / 2, height() - 8, inputLevelString);

  QString nonlinearFunctionString = tr("Nonlinear Function");

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
  pixelsWide = fm.horizontalAdvance(nonlinearFunctionString);
#else
  pixelsWide = fm.boundingRect(nonlinearFunctionString).width();
#endif

  qp.drawText(width() / 2 -pixelsWide / 2, 20, nonlinearFunctionString);

  qp.save();
  qp.rotate(-90);
  QString outputLevelString = tr("Output Level");

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
  pixelsWide = fm.horizontalAdvance(outputLevelString);
#else
  pixelsWide = fm.boundingRect(outputLevelString).width();
#endif

  qp.drawText(-pixelsWide / 2 - height() / 2, 12, outputLevelString);
  qp.restore();

  stepWidth = (width() - margin.left() - margin.right()) / 20;
  stepHeight = (height() - margin.top() - margin.bottom()) / 20;

  int i=0;
  while (i*stepHeight < height()-margin.top()-margin.bottom())
  {
    if (i % 5 == 0)
    {
      qp.setPen(penText);
      QString drText;
      qp.drawText(margin.left() - 35, 4 + i * stepHeight +
                  margin.top(), QString("%1").arg(maxOut -
                  (double)i / 20.0 * 2.0 * maxOut, 2, 'f', 2));
      qp.setPen(penBold);
    }
    else
    {
      qp.setPen(penThin);
    }

    qp.drawLine(margin.left(), i * stepHeight + margin.top(), width() - margin.right(),
      i * stepHeight + margin.top());

    i++;
  }

  i=0;

  while ((i * stepWidth) < (width() - margin.left() - margin.right()))
  {
    if (i % 5 == 0)
    {
      qp.setPen(penText);
      QString drText;
      qp.drawText(-4 + i * stepWidth + margin.left(), height() - margin.bottom() + 20,
        QString("%1").arg(-maxIn + (double)i / 20.0 * 2.0 * maxIn, 2, 'f', 2));
      qp.setPen(penBold);
    }
    else
    {
      qp.setPen(penThin);
    }

    qp.drawLine(i*stepWidth+margin.left(),
      margin.top(),
      i * stepWidth + margin.left(),
      height() - margin.bottom());

    i++;
  }

  qp.setPen(penZero);

  qp.drawLine(margin.left() + 2, 10 * stepHeight + margin.top(),
              width() - margin.right() - 2,
              10 * stepHeight + margin.top());

  qp.drawLine(10 * stepWidth + margin.left(),
    margin.top() + 2,
    10 * stepWidth + margin.left(),
    height() - margin.bottom() - 2);

  qp.setPen(QPen(QColor(255,255,255),1));
  qp.drawLine(margin.left(), margin.top(), width() - margin.right(), margin.top());
  qp.drawLine(margin.left(), margin.top(), margin.left(), height() - margin.bottom());
  qp.drawLine(margin.left(), height() - margin.bottom(),
              width() - margin.right(), height() - margin.bottom());
  qp.drawLine(width() - margin.right(), margin.top(),
              width() - margin.right(), height() - margin.bottom());
}

void NonlinearWidget::resizeEvent(QResizeEvent*)
{
  delete backbuffer;
  backbuffer = new QPixmap(width(),height());
  drawBackground();
  update(0,0,width(),height());
}

void NonlinearWidget::mouseMoveEvent(QMouseEvent *event)
{
  QPoint point(event->pos());

  if ((point.x() >= margin.left()) && (point.y() >= margin.top()) &&
      (point.x() <= width() - margin.right()) &&
      (point.y() <= height() - margin.bottom()))
  {
    infop = point;
    QFontMetrics metrics(infoFont,this);

    int graphWidth = width() - margin.left() - margin.right();
    int graphHeight = stepHeight * 20;

    double inValue = (point.x() - margin.left() - graphWidth / 2.0) /
      (graphWidth / 2.0) * maxIn;
    double outValue = -(point.y() - margin.right() - graphHeight / 2.0) /
      (graphHeight / 2.0) * maxOut;

    text = QString(tr("In: %1, Out: %2")).arg(inValue, 4, 'f', 2).arg(
      outValue, 4, 'f', 2);

    QRect infor = metrics.tightBoundingRect(text);
    infor.moveBottomLeft(infop);
    infor.adjust(-5,-5,5,5);

    update(backinfor);
    update(infor);
    backinfor = infor;
  }
}
