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

#include <QApplication>
#include <QTranslator>
#include <QSettings>
#include <QDir>

#include "mainwindow.h"
#include "processor.h"
#include "player.h"

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);

  QCoreApplication::setOrganizationName("Oleg Kapitonov");
  QCoreApplication::setApplicationName("tubeAmp Designer");

  QTranslator translator;
  QDir appdir(a.applicationDirPath());
  appdir.cdUp();
  appdir.cd("share/tubeAmp Designer/translations");
  translator.load("tAD_" + QLocale::system().name(), appdir.absolutePath());
  a.installTranslator(&translator);

  Player *playerInstance = new Player();
  if (playerInstance->connectToJack() == 1)
  {
    QMessageBox::critical(nullptr, "Error!",
      "Unable to connect to JACK server!");
    exit(1);
  }

  Processor *processorInstance = new Processor(playerInstance->getSampleRate());
  processorInstance->loadProfile(":/profiles/British Crunch.tapf");

  playerInstance->setProcessor(processorInstance);
  playerInstance->activate();

  MainWindow w(nullptr, processorInstance, playerInstance);
  w.showMaximized();

  int retVal = a.exec();

  delete playerInstance;
  delete processorInstance;

  int value = w.centralWidget->playerPanel->getInputLevelSliderValue();
  QSettings settings;
  settings.setValue("playerPanel/inputLevel", value - 50);

  return retVal;
}
