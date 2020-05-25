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

#include <QVBoxLayout>

#include "message_widget.h"

MessageWidget::MessageWidget(QWidget *parent) : QDialog(parent)
{
  setMinimumWidth(400);
  setMinimumHeight(80);

  QVBoxLayout *lay = new QVBoxLayout(this);
  messageLabel = new QLabel(this);
  lay->addWidget(messageLabel);
  messageLabel->setAlignment(Qt::AlignCenter);

  progressBar = new QProgressBar(this);
  lay->addWidget(progressBar);
  progressBar->setValue(0);
}

void MessageWidget::setTitle(QString title)
{
  setWindowTitle(title);
}

void MessageWidget::setMessage(QString message)
{
  messageLabel->setText(message);
}

void MessageWidget::setProgressValue(int value)
{
  progressBar->setValue(value);
}
