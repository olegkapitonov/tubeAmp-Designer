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

#ifndef AMPNONLINEAREDITWIDGET_H
#define AMPNONLINEAREDITWIDGET_H

#include <QtWidgets/QWidget>
#include <QPushButton>

#include "block_edit_widget.h"
#include "nonlinear_widget.h"
#include "processor.h"
#include "slide_box_widget.h"

class AmpNonlinearEditWidget : public BlockEditWidget
{
  Q_OBJECT

public:
  AmpNonlinearEditWidget(QWidget *parent = nullptr, Processor *prc = nullptr);

  virtual void recalculate();
  virtual void resetControls();
  virtual void updateControls();

private:
  Processor *processor;
  NonlinearWidget *nonlinear;

  SlideBoxWidget *biasSlide;
  SlideBoxWidget *uporSlide;
  SlideBoxWidget *kregSlide;
  SlideBoxWidget *levelSlide;
  SlideBoxWidget *sagCoeffSlide;
  SlideBoxWidget *sagTimeSlide;
  SlideBoxWidget *masterOutputLevelSlide;

public slots:
  void biasChanged(float value);
  void uporChanged(float value);
  void kregChanged(float value);
  void levelChanged(float value);
  void sagCoeffChanged(float value);
  void sagTimeChanged(float value);
  void masterOutputLevelChanged(float value);

};

#endif // AMPNONLINEAREDITWIDGET_H
