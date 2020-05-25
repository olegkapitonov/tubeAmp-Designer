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

#include "equalizer_widget.h"

EqualizerWidget::EqualizerWidget(QWidget *parent) : QWidget(parent)
{
  QPalette Pal(palette());
  Pal.setColor(QPalette::Background, Qt::black);
  setAutoFillBackground(true);
  setPalette(Pal);

  backbuffer = new QPixmap(100, 100);

  maxDb = 20;

  activePoint = 0;

  setMaximumHeight(450);
  setMinimumHeight(450);

  setMouseTracking(true);

  resetEq();

  margin.setLeft(50);
  margin.setRight(20);
  margin.setTop(40);
  margin.setBottom(50);

  stepHeight = 10;
  dbInStep = 20;
}

void EqualizerWidget::paintEvent(QPaintEvent * event)
{
  QPainter qp(this);
  qp.setRenderHint(QPainter::Antialiasing);

  for (QRegion::const_iterator rects = event->region().begin();
       rects!= event->region().end(); rects++)
  {
    QRect r = *rects;
    qp.drawPixmap(r,*backbuffer,r);
  }

  double farPointDb = dbValuesEq[dbValuesEq.size() - 1] - 40.0;
  double farPointFLog = fLogValuesEq[fLogValuesEq.size() - 1] + 1.0;

  QVector<double> dbValuesEqSym((dbValuesEq.size() + 1) * 2);
  QVector<double> fLogValuesEqSym((fLogValuesEq.size() + 1) * 2);

  dbValuesEqSym[0] = farPointDb;
  fLogValuesEqSym[0] = -farPointFLog;
  dbValuesEqSym[dbValuesEqSym.size() - 1] = farPointDb;
  fLogValuesEqSym[fLogValuesEqSym.size() - 1] = farPointFLog;

  for (int i = 1; i < dbValuesEqSym.size() / 2; i++)
  {
    dbValuesEqSym[i + dbValuesEqSym.size() / 2 - 1] = dbValuesEq[i - 1];
    dbValuesEqSym[-i + dbValuesEqSym.size() / 2] = dbValuesEq[i - 1];
    fLogValuesEqSym[i + dbValuesEqSym.size() / 2 - 1] = fLogValuesEq[i - 1];
    fLogValuesEqSym[-i + dbValuesEqSym.size() / 2] = -fLogValuesEq[i - 1];;
  }

  gsl_interp_accel *acc_eq = gsl_interp_accel_alloc ();
  gsl_spline *spline_eq = gsl_spline_alloc (gsl_interp_cspline, fLogValuesEqSym.count());

  gsl_spline_init (spline_eq, fLogValuesEqSym.data(), dbValuesEqSym.data(),
                   fLogValuesEqSym.count());

  gsl_interp_accel *acc_fr = gsl_interp_accel_alloc();
  gsl_spline *spline_fr = gsl_spline_alloc(gsl_interp_cspline, fLogValuesFr.count());

  gsl_spline_init(spline_fr, fLogValuesFr.data(), dbValuesFr.data(),
                  fLogValuesFr.count());

  QPolygon polyline1(width() - margin.left() - margin.right());
  QPolygon polyline2(width() - margin.left() - margin.right());

  int maxFr = 10000;

  for (int i=0; i < width() - margin.left() - margin.right(); i++)
  {
    double interpolatedFreq = pow(10, 1 + (double)(i) / (double)stepWidth);

    if (interpolatedFreq > 20000.0) interpolatedFreq = 20000.0;

    double interpolatedDbEq = gsl_spline_eval(spline_eq,
                                              log10(interpolatedFreq),
                                              acc_eq);
    double interpolatedDbFr = gsl_spline_eval(spline_fr,
                                              log10(interpolatedFreq),
                                              acc_fr) + interpolatedDbEq;

    int yEq = margin.top() - interpolatedDbEq * 10.0 / dbInStep * stepHeight
      + maxDb * stepHeight * 10.0 / dbInStep;

    int yFr = margin.top() - interpolatedDbFr * 10.0 / dbInStep * stepHeight
      + maxDb * stepHeight * 10.0 / dbInStep;

    if (yEq > (height() - margin.bottom()))
    {
      yEq = height() - margin.bottom();
    }

    if (yEq < (margin.top()))
    {
      yEq = margin.top();
    }

    polyline1.setPoint(i, i + margin.left(), yEq);
    polyline2.setPoint(i, i + margin.left(), yFr);

    if (maxFr > yFr)
    {
      maxFr = yFr;
    }
  }

  for (int i=0; i < width() - margin.left() - margin.right(); i++)
  {
    int x, y;
    polyline2.point(i, &x, &y);

    y += -maxFr + margin.top() + maxDb * stepHeight * 10.0 / dbInStep;

    if (y > (height() - margin.bottom()))
    {
      y = height() - margin.bottom();
    }

    if (y < (margin.top()))
    {
      y = margin.top();
    }

    polyline2.setPoint(i, x, y);
  }

  gsl_spline_free(spline_eq);
  gsl_interp_accel_free(acc_eq);

  gsl_spline_free(spline_fr);
  gsl_interp_accel_free(acc_fr);

  qp.setPen(QPen(QColor(0,255,0)));
  qp.drawPolyline(polyline1);

  qp.setPen(QPen(QColor(150,255,255)));
  qp.drawPolyline(polyline2);


  qp.setPen(QPen(QColor(255,255,255)));
  infoFont = QFont("Sans",12);
  qp.setFont(infoFont);

  qp.drawText(infop,text);

  for (int i = 0; i < fLogValuesEq.count() - 1; i++)
  {
    int x = (fLogValuesEq[i] - 1.0) * stepWidth + margin.left();
    int y = (-dbValuesEq[i] + maxDb) * 10 / dbInStep * stepHeight + margin.top();

    if (i != activePoint)
    {
      qp.setBrush(QBrush(QColor(0,200,0)));
      qp.setPen(QPen(QColor(0,200,0)));
    }
    else
    {
      qp.setPen(QPen(QColor(200,255,200)));
      qp.setBrush(QBrush(QColor(200,255,200)));
    }

    qp.drawEllipse(x - 10, y - 10, 20, 20);
  }
}

void EqualizerWidget::mouseMoveEvent(QMouseEvent *event)
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
    infor.adjust(-5, -5, 5, 5);

    update(backinfor);
    update(infor);
    backinfor = infor;
  }

  if (event->buttons() == Qt::LeftButton)
  {
    int dx = point.x() - mousePressPoint.x();
    int dy = point.y() - mousePressPoint.y();

    int newY = (- activeDb + (double)dy / 10.0 * dbInStep / stepHeight) *
      10 / dbInStep * stepHeight +
      maxDb * 10 / dbInStep * stepHeight + margin.top();

    if ((newY <= height() - margin.bottom()) && (newY >= margin.top()))
    {
      if ((activePoint != 0) && (activePoint != fLogValuesEq.count() - 2))
      {
        double newF = activeFLog + (double)dx / (double)stepWidth;
        if ((newF < fLogValuesEq[activePoint + 1]) &&
            (newF > fLogValuesEq[activePoint - 1]))
        {
          fLogValuesEq[activePoint] = newF;
        }
      }

      double dyDb = (double)dy / 10.0 * dbInStep / stepHeight;

      dbValuesEq[activePoint] = activeDb - dyDb;

      if (activePoint == (fLogValuesEq.count() - 2))
      {
        dbValuesEq[dbValuesEq.size() - 1] = activeDb - dyDb;
      }

      update(0, 0, width(), height());
      isResponseChanged = true;
    }
  }
  else if (event->buttons() == Qt::NoButton)
  {
    int minDistance = INT_MAX;
    int nearestPoint = 0;

    for (int i = 0; i < fLogValuesEq.count() - 1; i++)
    {
      int x = (fLogValuesEq[i] - 1.0) * stepWidth + margin.left();
      int y = (-dbValuesEq[i] + maxDb) * stepHeight / dbInStep * 10 + margin.top();

      int distanceToPoint = sqrt(pow(x - point.x(), 2) + pow(y - point.y(), 2));

      if (distanceToPoint < minDistance)
      {
        minDistance = distanceToPoint;
        nearestPoint = i;
      }
    }

    activePoint = nearestPoint;
    update(QRect(0, 0, width(), height()));
  }
}

void EqualizerWidget::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::LeftButton)
  {
    mousePressPoint = event->pos();
    activeFLog = fLogValuesEq[activePoint];
    activeDb = dbValuesEq[activePoint];
  }
  else if (event->button() == Qt::RightButton)
  {
    if ((activePoint != 0) && (activePoint != (fLogValuesEq.count() - 1)
      && fLogValuesEq.count() > 3))
    {
      fLogValuesEq.remove(activePoint, 1);
      dbValuesEq.remove(activePoint, 1);
      update(0, 0, width(), height());
      isResponseChanged = true;
    }
  }
}

void EqualizerWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
  if (event->button() == Qt::LeftButton)
  {
    QPoint point(event->pos());

    double db = maxDb - (double)(infop.y()-margin.top()) /
      (double)stepHeight * (double)dbInStep / 10.0;

    double fLog = 1 + (double)(infop.x()-margin.left())/(double)stepWidth;

    int insertPos = 0;

    for (int i = 0; i < fLogValuesEq.count(); i++)
    {
      if (fLogValuesEq[i] >= fLog)
      {
        insertPos = i;
        break;
      }
    }

    if (fLogValuesEq[insertPos] != fLog)
    {
      fLogValuesEq.insert(insertPos, fLog);
      dbValuesEq.insert(insertPos, db);
      activePoint = insertPos;

      update(0, 0, width(), height());
      isResponseChanged = true;
    }
  }
}

void EqualizerWidget::drawBackground()
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
  while (i*stepHeight < height()-margin.top()-margin.bottom())
  {
    if (i % 5 == 0)
    {
      qp.setPen(penText);
      QString drText;
      qp.drawText(margin.left() - 25, 4 + i * stepHeight +
        margin.top(), QString("%1").arg(maxDb - dbInStep/2*(i / 5)));
      qp.setPen(penBold);
    }
    else
    {
      qp.setPen(penThin);
    }

    qp.drawLine(margin.left(), i * stepHeight + margin.top(), width() - margin.right(),
      i * stepHeight+margin.top());

    i++;
  }

  i=0;

  while ((i * stepWidth) < (width() - margin.left() - margin.right()))
  {
    for (int x = 1; x < 10; x++)
    {
      if ((stepWidth * log10(x) + i * stepWidth + margin.left()) <=
         (width() - margin.right()))
      {
        if (x == 1)
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

  qp.setPen(QPen(QColor(255, 255, 255), 1));
  qp.drawLine(margin.left(), margin.top(), width() - margin.right(), margin.top());
  qp.drawLine(margin.left(), margin.top(), margin.left(), height() - margin.bottom());
  qp.drawLine(margin.left(), height() - margin.bottom(), width() -
              margin.right(), height() - margin.bottom());
  qp.drawLine(width() - margin.right(), margin.top(), width() - margin.right(),
              height() - margin.bottom());
}

void EqualizerWidget::resizeEvent(QResizeEvent*)
{
  delete backbuffer;
  backbuffer = new QPixmap(width(), height());
  drawBackground();
  update(0,0,width(),height());
}

void EqualizerWidget::mouseReleaseEvent(QMouseEvent *)
{
  if (isResponseChanged)
  {
    emit responseChanged();
    isResponseChanged = false;
  }
}

void EqualizerWidget::resetEq()
{
  fLogValuesEq.resize(4);
  dbValuesEq.resize(4);

  fLogValuesEq[0] = log10(10.0);
  fLogValuesEq[1] = log10(1000.0);
  fLogValuesEq[2] = log10(20000.0);
  fLogValuesEq[3] = log10(22000.0);

  dbValuesEq[0] = 0.0;
  dbValuesEq[1] = 0.0;
  dbValuesEq[2] = 0.0;
  dbValuesEq[3] = 0.0;
}
