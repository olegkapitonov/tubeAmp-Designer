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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

#include "processor.h"
#include "player.h"
#include "centralwidget.h"
#include "profiler_dialog.h"
#include "convolver_dialog.h"
#include "deconvolver_dialog.h"

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent, Processor *prc, Player *plr);

  CentralWidget *centralWidget;

private:
  QAction *actionOpen;
  QAction *actionSave;
  QAction *actionSave_As;
  QAction *actionQuit;
  QAction *actionAbout;
  QAction *actionProfiler;
  QAction *actionConvolver;
  QAction *actionDeconvolver;

  QMenuBar *menuBar;
  QMenu *menuFile;
  QMenu *menuTools;
  QMenu *menuHelp;
  QStatusBar *statusBar;

  ProfilerDialog *profilerDialog;
  ConvolverDialog *convolverDialog;
  DeconvolverDialog *deconvolverDialog;

  Processor *processor;
  Player *player;

private slots:
  void actionOpenTriggered();
  void actionSaveTriggered();
  void actionSaveAsTriggered();
  void actionProfilerTriggered();
  void actionConvolverTriggered();
  void actionDeconvolverTriggered();
  void actionQuitTriggered();
  void actionAboutTriggered();

  void profilerDialogAccepted();
};

#endif // MAINWINDOW_H
