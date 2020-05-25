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

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>

#include "load_dialog.h"

LoadDialog::LoadDialog(QWidget *parent) : QDialog(parent)
{
  setMinimumWidth(600);
  setWindowTitle(tr("Open sound files"));

  QGridLayout *lay = new QGridLayout(this);

  QLabel *diLabel = new QLabel(tr("Direct Input from guitar"), this);
  lay->addWidget(diLabel, 0, 0, 1, 2);

  diFilenameEdit = new QLineEdit(this);
  lay->addWidget(diFilenameEdit, 1, 0, 1, 1);

  QPushButton *diFilenameButton = new QPushButton(tr("Open"), this);
  lay->addWidget(diFilenameButton, 1, 1, 1, 1);

  connect(diFilenameButton, &QPushButton::clicked, this, &LoadDialog::diFilenameButtonClicked);
  connect(diFilenameEdit, &QLineEdit::textEdited, this, &LoadDialog::lineEditEdited);

  QLabel *refLabel = new QLabel(tr("Reference sound file"), this);
  lay->addWidget(refLabel, 2, 0, 1, 2);

  refFilenameEdit = new QLineEdit(this);
  lay->addWidget(refFilenameEdit, 3, 0, 1, 1);

  QPushButton *refFilenameButton = new QPushButton(tr("Open"), this);
  lay->addWidget(refFilenameButton, 3, 1, 1, 1);

  connect(refFilenameButton, &QPushButton::clicked, this, &LoadDialog::refFilenameButtonClicked);
  connect(refFilenameEdit, &QLineEdit::textEdited, this, &LoadDialog::lineEditEdited);

  QWidget *buttonsContainer = new QWidget(this);
  lay->addWidget(buttonsContainer, 4, 0, 1, 2);

  QHBoxLayout *containerLay = new QHBoxLayout(buttonsContainer);
  okButton = new QPushButton(tr("OK"), buttonsContainer);
  containerLay->addWidget(okButton);
  okButton->setMaximumWidth(80);
  okButton->setEnabled(false);

  connect(okButton, &QPushButton::clicked, this, &LoadDialog::okButtonClicked);

  QPushButton *cancelButton = new QPushButton(tr("Cancel"), buttonsContainer);
  containerLay->addWidget(cancelButton);
  cancelButton->setMaximumWidth(80);

  connect(cancelButton, &QPushButton::clicked, this, &LoadDialog::cancelButtonClicked);
}

void LoadDialog::diFilenameButtonClicked()
{
  diFileName = QFileDialog::getOpenFileName(this, tr("Open DI File"),
                                            QString(),
                                            tr("Sound files (*.wav *.ogg *.flac)"));
  if (!diFileName.isEmpty())
  {
    diFilenameEdit->setText(diFileName);
  }

  checkFilenames();
}

void LoadDialog::refFilenameButtonClicked()
{
  refFileName = QFileDialog::getOpenFileName(this,
                                             tr("Open Reference File"),
                                             QString(),
                                             tr("Sound files (*.wav *.ogg *.flac)"));

  if (!refFileName.isEmpty())
  {
    refFilenameEdit->setText(refFileName);
  }

  checkFilenames();
}

void LoadDialog::cancelButtonClicked()
{
  reject();
}

void LoadDialog::okButtonClicked()
{
  if ((!QFile(diFileName).exists()) || (!QFile(refFileName).exists()))
  {
    QMessageBox::warning(this, tr("Attention!"), tr("DI or Reference file not found!"));
    return;
  }
  accept();
}

QString LoadDialog::getDiFileName()
{
  return diFileName;
}

QString LoadDialog::getRefFileName()
{
  return refFileName;
}

void LoadDialog::checkFilenames()
{
  if (!(diFilenameEdit->text().isEmpty() || refFilenameEdit->text().isEmpty()))
  {
    okButton->setEnabled(true);
  }
}

void LoadDialog::lineEditEdited(QString)
{
  checkFilenames();
}
