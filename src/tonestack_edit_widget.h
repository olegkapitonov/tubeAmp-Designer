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

#ifndef TONESTACKEDITWIDGET_H
#define TONESTACKEDITWIDGET_H

#include <QtWidgets/QWidget>
#include <QPushButton>

#include "block_edit_widget.h"
#include "freq_response_widget.h"
#include "processor.h"
#include "slide_box_widget.h"

class TonestackEditWidget : public BlockEditWidget
{
  Q_OBJECT

public:
  TonestackEditWidget(QWidget *parent = nullptr, Processor *prc = nullptr);

  virtual void recalculate();
  virtual void resetControls();

private:
  Processor *processor;
  FreqResponseWidget * freqResponse;

  SlideBoxWidget *bassFreqSlide;
  SlideBoxWidget *bassBandSlide;
  SlideBoxWidget *middleFreqSlide;
  SlideBoxWidget *middleBandSlide;
  SlideBoxWidget *trebleFreqSlide;
  SlideBoxWidget *trebleBandSlide;

public slots:
  void bassFreqChanged(float value);
  void bassBandChanged(float value);

  void middleFreqChanged(float value);
  void middleBandChanged(float value);

  void trebleFreqChanged(float value);
  void trebleBandChanged(float value);

};

#endif // TONESTACKEDITWIDGET_H
