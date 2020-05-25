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

#ifndef PREAMPFILTEREDITWIDGET_H
#define PREAMPFILTEREDITWIDGET_H

#include <QtWidgets/QWidget>
#include <QPushButton>

#include "block_edit_widget.h"
#include "processor.h"
#include "equalizer_widget.h"
#include "freq_response_widget.h"
#include "file_resampling_thread.h"
#include "message_widget.h"

class PreampFilterEditWidget : public BlockEditWidget
{
  Q_OBJECT

public:
  PreampFilterEditWidget(QWidget *parent = nullptr, Processor *prc = nullptr);

private:
  enum DisableStatus {STAT_DISABLED, STAT_ENABLED};

  QPushButton *loadButton;
  QPushButton *saveButton;
  QPushButton *resetButton;
  QPushButton *applyButton;
  QPushButton *disableButton;

  Processor *processor;
  EqualizerWidget *equalizer;

  FileResamplingThread *fileResamplingThread;

  MessageWidget *msg;

  DisableStatus disableStatus;

  virtual void recalculate();
  virtual void resetControls();

public slots:
  void responseChangedSlot();
  void applyButtonClicked();
  void resetButtonClicked();
  void saveButtonClicked();
  void loadButtonClicked();
  void disableButtonClicked();

  void fileResamplingThreadFinished();

};

#endif // PREAMPFILTEREDITWIDGET_H
