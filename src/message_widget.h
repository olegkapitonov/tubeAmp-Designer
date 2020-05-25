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

#ifndef MESSAGE_WIDGET_H
#define MESSAGE_WIDGET_H

#include <QDialog>
#include <QProgressBar>
#include <QLabel>

class MessageWidget : public QDialog
{
  Q_OBJECT

public:
  MessageWidget(QWidget *parent = nullptr);

  void setTitle(QString title);
  void setMessage(QString message);
  void setProgressValue(int value);

private:
  QProgressBar *progressBar;
  QLabel *messageLabel;
};

#endif //MESSAGE_WIDGET_H
