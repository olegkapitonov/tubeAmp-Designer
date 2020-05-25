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
#include <QPainter>
#include <QPaintEvent>

#include <cmath>
#include <gsl/gsl_spline.h>

#include "freq_response_widget.h"

FreqResponseWidget::FreqResponseWidget(QWidget *parent) :
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

  fLogValues.resize(3);
  dbValues.resize(3);

  fLogValues[0] = log10(10.0);
  fLogValues[1] = log10(1000.0);
  fLogValues[2] = log10(20000.0);

  dbValues[0] = 0.0;
  dbValues[1] = 0.0;
  dbValues[2] = 0.0;

  maxDb = 0;

  margin.setLeft(50);
  margin.setRight(20);
  margin.setTop(40);
  margin.setBottom(50);

  stepHeight = 10;
  dbInStep = 20;
}

void FreqResponseWidget::paintEvent(QPaintEvent * event)
{
  QPainter qp(this);

  for (QRegion::const_iterator rects = event->region().begin();
       rects!= event->region().end(); rects++)
  {
    QRect r = *rects;
    qp.drawPixmap(r,*backbuffer,r);
  }

  gsl_interp_accel *acc = gsl_interp_accel_alloc ();
  gsl_spline *spline = gsl_spline_alloc (gsl_interp_cspline, fLogValues.count());

  gsl_spline_init (spline, fLogValues.data(), dbValues.data(), fLogValues.count());

  QPolygon polyline1(width() - margin.left() - margin.right());

  for (int i=0; i < width() - margin.left() - margin.right(); i++)
  {
    double interpolatedFreq = pow(10, 1 + (double)(i) / (double)stepWidth);

    if (interpolatedFreq > 20000.0) interpolatedFreq = 20000.0;

    double interpolatedDb = gsl_spline_eval(spline, log10(interpolatedFreq), acc);

    int y = margin.top() - interpolatedDb * 10.0 / dbInStep * stepHeight +
      maxDb * stepHeight * 10.0 / dbInStep;

    if (y > (height() - margin.bottom()))
    {
      y = height() - margin.bottom();
    }

    if (y < (margin.top()))
    {
      y = margin.top();
    }

    polyline1.setPoint(i, i + margin.left(), y);
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

void FreqResponseWidget::drawBackground()
{
  QPainter qp(backbuffer);
  qp.fillRect(0,0,width(),height(),QBrush(QColor(0,0,0)));

  QPen penThin(QColor(80,80,80));
  QPen penBold(QColor(150,150,150),1);
  QPen penText(QColor(255,255,255));

  QFontMetrics fm(QFont("Sans",10));
  qp.setFont(QFont("Sans",10));
  qp.setPen(penText);
  QString frequencyResponseString = tr("Frequency Response");

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
  int pixelsWide = fm.horizontalAdvance(frequencyResponseString);
#else
  int pixelsWide = fm.boundingRect(frequencyResponseString).width();
#endif

  qp.drawText(width() / 2 -pixelsWide / 2, 20, frequencyResponseString);
  QString frequencyString = tr("Frequency, Hz");

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
  pixelsWide = fm.horizontalAdvance(frequencyString);
#else
  pixelsWide = fm.boundingRect(frequencyString).width();
#endif

  qp.drawText(width() / 2 -pixelsWide / 2, height() - 8, frequencyString);

  qp.save();
  qp.rotate(-90);
  QString magnitudeString = tr("Magnitude, dB");

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
  pixelsWide = fm.horizontalAdvance(magnitudeString);
#else
  pixelsWide = fm.boundingRect(magnitudeString).width();
#endif

  qp.drawText(-pixelsWide / 2 - height() / 2, 12, magnitudeString);
  qp.restore();

  stepWidth = (width() - margin.left() - margin.right()) / 3;
  stepWidth -= (1 - log10(8)) * stepWidth;

  int i=0;
  while ((i * stepHeight) < (height() - margin.top() - margin.bottom()))
  {
    if (i % 5 == 0)
    {
      qp.setPen(penText);
      QString drText;
      qp.drawText(margin.left() - 25, 4 + i * stepHeight +
        margin.top(), QString("%1").arg(maxDb - dbInStep/2 * (i / 5)));
      qp.setPen(penBold);
    }
    else
    {
      qp.setPen(penThin);
    }

    qp.drawLine(margin.left(), i * stepHeight + margin.top(),
                width() - margin.right(),
                i * stepHeight + margin.top());

    i++;
  }

  i=0;

  while ((i * stepWidth) < (width() - margin.left() - margin.right()))
  {
    for (int x = 1; x < 10; x++)
    {
      if (stepWidth * log10(x) + i * stepWidth +
          margin.left() <= (width() - margin.right()))
      {
        if (x==1)
        {
          qp.setPen(penBold);
        }
        else
        {
          qp.setPen(penThin);
        }

        qp.drawLine(stepWidth * log10(x) + i * stepWidth + margin.left(),
          margin.top(),
          stepWidth * log10(x) + i * stepWidth + margin.left(),
          height() - margin.bottom());
      }
    }
    qp.setPen(penText);
    QString drText;
    qp.drawText(margin.left() + i * stepWidth - 4, height() - margin.bottom() + 20,
      QString("%1").arg((int)(10 * pow(10, i))));

    i++;
  }

  qp.setPen(QPen(QColor(255,255,255),1));
  qp.drawLine(margin.left(), margin.top(), width() - margin.right(), margin.top());
  qp.drawLine(margin.left(), margin.top(), margin.left(), height() - margin.bottom());
  qp.drawLine(margin.left(), height() - margin.bottom(), width() - margin.right(),
              height() - margin.bottom());
  qp.drawLine(width() - margin.right(), margin.top(), width() - margin.right(),
              height() - margin.bottom());
}

void FreqResponseWidget::resizeEvent(QResizeEvent*)
{
  delete backbuffer;
  backbuffer = new QPixmap(width(),height());
  drawBackground();
  update(0,0,width(),height());
}

void FreqResponseWidget::mouseMoveEvent(QMouseEvent *event)
{
  QPoint point(event->pos());

  if ((point.x() >= margin.left()) && (point.y() >= margin.top()) &&
       (point.x() <= width() - margin.right()) &&
       (point.y() <= height() - margin.bottom()))
  {
    infop = point;
    QFontMetrics metrics(infoFont,this);

    double Lvalue = maxDb - (double)(infop.y() - margin.top()) /
      (double)stepHeight * (double)dbInStep / 10.0;

    text = QString(tr("f: %1 Hz, k: %2 dB")).arg(
      pow(10, 1 + (double)(infop.x() - margin.left()) /
      (double)stepWidth), 4, 'f', 2).arg(Lvalue,4,'g',3);

    QRect infor = metrics.tightBoundingRect(text);
    infor.moveBottomLeft(infop);
    infor.adjust(-5,-5,5,5);

    update(backinfor);
    update(infor);
    backinfor = infor;
  }
}
