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

#include <QFileDialog>
#include <QMessageBox>

#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent, Processor *prc, Player *plr) :
  QMainWindow(parent)
{
  processor = prc;
  player = plr;
  resize(615, 410);

  actionOpen = new QAction(QApplication::style()->standardIcon(
    QStyle::SP_DialogOpenButton), "Open", this);
  connect(actionOpen, &QAction::triggered, this, &MainWindow::actionOpenTriggered);

  actionSave = new QAction(QApplication::style()->standardIcon(
    QStyle::SP_DialogSaveButton), "Save", this);
  connect(actionSave, &QAction::triggered, this, &MainWindow::actionSaveTriggered);

  actionSave_As = new QAction(this);
  connect(actionSave_As, &QAction::triggered, this, &MainWindow::actionSaveAsTriggered);

  actionQuit = new QAction(QApplication::style()->standardIcon(
    QStyle::SP_DialogCloseButton), "Close",this);
  connect(actionQuit, &QAction::triggered, this, &MainWindow::actionQuitTriggered);

  actionAbout = new QAction(QApplication::style()->standardIcon(
    QStyle::SP_DialogHelpButton), "About",this);
  connect(actionAbout, &QAction::triggered, this, &MainWindow::actionAboutTriggered);

  actionProfiler = new QAction(this);
  connect(actionProfiler, &QAction::triggered, this, &MainWindow::actionProfilerTriggered);

  actionConvolver= new QAction(this);
  connect(actionConvolver, &QAction::triggered, this, &MainWindow::actionConvolverTriggered);

  actionDeconvolver = new QAction(this);
  connect(actionDeconvolver, &QAction::triggered, this, &MainWindow::actionDeconvolverTriggered);

  centralWidget = new CentralWidget(this, processor, player);
  setCentralWidget(centralWidget);

  menuBar = new QMenuBar(this);
  menuBar->setGeometry(QRect(0, 0, 615, 30));
  menuFile = new QMenu(menuBar);
  menuTools = new QMenu(menuBar);
  menuHelp = new QMenu(menuBar);
  setMenuBar(menuBar);

  statusBar = new QStatusBar(this);
  setStatusBar(statusBar);

  menuBar->addAction(menuFile->menuAction());
  menuBar->addAction(menuTools->menuAction());
  menuBar->addAction(menuHelp->menuAction());
  menuFile->addAction(actionOpen);
  menuFile->addAction(actionSave);
  menuFile->addAction(actionSave_As);
  menuFile->addSeparator();
  menuFile->addAction(actionQuit);
  menuTools->addAction(actionProfiler);
  menuTools->addAction(actionConvolver);
  menuTools->addAction(actionDeconvolver);
  menuHelp->addAction(actionAbout);

  setWindowTitle(QApplication::translate("MainWindow", "tubeAmp Designer", nullptr));
  setWindowIcon(QIcon(":/icons/logo.png"));

  actionOpen->setText(QApplication::translate("MainWindow", "Open", nullptr));
  actionSave->setText(QApplication::translate("MainWindow", "Save", nullptr));
  actionSave_As->setText(QApplication::translate("MainWindow", "Save As", nullptr));
  actionQuit->setText(QApplication::translate("MainWindow", "Quit", nullptr));
  actionAbout->setText(QApplication::translate("MainWindow", "About", nullptr));
  actionProfiler->setText(QApplication::translate("MainWindow", "Profiler", nullptr));
  actionConvolver->setText(QApplication::translate("MainWindow", "Convolver", nullptr));
  actionDeconvolver->setText(QApplication::translate("MainWindow",
                                                     "Deconvolver", nullptr));
  menuFile->setTitle(QApplication::translate("MainWindow", "File", nullptr));
  menuTools->setTitle(QApplication::translate("MainWindow", "Tools", nullptr));
  menuHelp->setTitle(QApplication::translate("MainWindow", "Help", nullptr));

  profilerDialog = new ProfilerDialog(processor, player, centralWidget->playerPanel, this);
  connect(profilerDialog, &ProfilerDialog::accepted,
    this,   &MainWindow::profilerDialogAccepted);

  convolverDialog = new ConvolverDialog(processor, this);
  deconvolverDialog = new DeconvolverDialog(processor, this);
}

void MainWindow::actionOpenTriggered()
{
  QDir profilesDir(QCoreApplication::applicationDirPath());
  profilesDir.cdUp();
  profilesDir.cd("share/tubeAmp Designer/profiles");
  QString newProfileFileName =
    QFileDialog::getOpenFileName(this,
                                 "Open profile file",
                                 profilesDir.absolutePath(),
                                 "tubeAmp profiles (*.tapf)");

  if (!newProfileFileName.isEmpty())
  {
    Player::PlayerStatus rememberStatus = player->status;
    player->setStatus(Player::PlayerStatus::PS_STOP);
    processor->loadProfile(newProfileFileName);
    setWindowTitle("tubeAmp Designer — " + QFileInfo(newProfileFileName).baseName());
    centralWidget->reloadBlocks();
    player->setStatus(rememberStatus);
  }
}

void MainWindow::actionSaveTriggered()
{
  if ((processor->isPreampCorrectionEnabled()) || (processor->isCabinetCorrectionEnabled()))
  {
    int ret = QMessageBox::warning(this, tr("Warning!"),
                                  tr("Equalizer settings changed\n"
                                  "and will be applied before saving!\n"
                                  "This can't be undone!"),
                                  QMessageBox::Save | QMessageBox::Cancel,
                                  QMessageBox::Save);

    if (ret == QMessageBox::Cancel)
    {
      return;
    }

    if (processor->isPreampCorrectionEnabled())
    {
      processor->applyPreampCorrection();
      processor->resetPreampCorrection();
    }

    if (processor->isCabinetCorrectionEnabled())
    {
      processor->applyCabinetSumCorrection();
      processor->resetCabinetSumCorrection();
    }

    centralWidget->reloadBlocks();
  }

  if (!processor->saveProfile(processor->getProfileFileName()))
  {
    QMessageBox::critical(this, tr("Error!"),
                        tr("Can't save profile!"));

    actionSaveAsTriggered();
  }
}

void MainWindow::actionSaveAsTriggered()
{
  if ((processor->isPreampCorrectionEnabled()) || (processor->isCabinetCorrectionEnabled()))
  {
    int ret = QMessageBox::warning(this, tr("Warning!"),
                                  tr("Equalizer settings changed\n"
                                  "and will be applied before saving!\n"
                                  "This can't be undone!"),
                                  QMessageBox::Save | QMessageBox::Cancel,
                                  QMessageBox::Save);

    if (ret == QMessageBox::Cancel)
    {
      return;
    }

    if (processor->isPreampCorrectionEnabled())
    {
      processor->applyPreampCorrection();
      processor->resetPreampCorrection();
    }

    if (processor->isCabinetCorrectionEnabled())
    {
      processor->applyCabinetSumCorrection();
      processor->resetCabinetSumCorrection();
    }

    centralWidget->reloadBlocks();
  }

  QString saveProfileFileName =
    QFileDialog::getSaveFileName(this,
                                "Save profile file",
                                QString(),
                                "tubeAmp profiles (*.tapf)");

  if (!saveProfileFileName.isEmpty())
  {
    processor->saveProfile(saveProfileFileName);
    setWindowTitle("tubeAmp Designer — " + QFileInfo(saveProfileFileName).baseName());
  }
}

void MainWindow::actionProfilerTriggered()
{
  profilerDialog->show();
}

void MainWindow::profilerDialogAccepted()
{
  centralWidget->updateBlocks();
  setWindowTitle("tubeAmp Designer — New Profile");
}

void MainWindow::actionQuitTriggered()
{
  close();
}

void MainWindow::actionConvolverTriggered()
{
  convolverDialog->open();
}

void MainWindow::actionDeconvolverTriggered()
{
  deconvolverDialog->open();
}

void MainWindow::actionAboutTriggered()
{
  QMessageBox::about(this, tr("About"),
                     tr("tubeAmp Designer - Virtual guitar"
                     " amplifier and profiler.\n2020 Oleg Kapitonov"));
}
