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

#include "file_resampling_thread.h"
#include "math_functions.h"

FileResamplingThread::FileResamplingThread()
{
  stereoMode = false;
}

void FileResamplingThread::run()
{
  if (!filename.isEmpty())
  {
    loadFile(filename.toUtf8().constData());
  }
}

void FileResamplingThread::loadFile(const char *filename)
{
  SF_INFO sfinfo;
  sfinfo.format = 0;

  SNDFILE *sndFile = sf_open(filename, SFM_READ, &sfinfo);
  if (sndFile != NULL)
  {
    QVector<float> tempBuffer(sfinfo.frames * sfinfo.channels);
    sf_readf_float(sndFile, tempBuffer.data(), sfinfo.frames);

    sf_close(sndFile);

    QVector<float> notResampledBufferL(sfinfo.frames);
    QVector<float> notResampledBufferR(sfinfo.frames);

    for (int i = 0; i < sfinfo.frames * sfinfo.channels; i += sfinfo.channels)
    {
      float sumFrame = 0.0;
      if (stereoMode && (sfinfo.channels > 1))
      {
        for (int j = 1; j < sfinfo.channels; j++)
        {
          sumFrame += tempBuffer[i + j];
        }
        sumFrame /= sfinfo.channels - 1;
        notResampledBufferL[i / sfinfo.channels] = tempBuffer[i];
        notResampledBufferR[i / sfinfo.channels] = sumFrame;
      }
      else
      {
        for (int j = 0; j < sfinfo.channels; j++)
        {
          sumFrame += tempBuffer[i + j];
        }
        sumFrame /= sfinfo.channels;
        notResampledBufferL[i / sfinfo.channels] = sumFrame;
      }
    }

    dataL = resample_vector(notResampledBufferL, sfinfo.samplerate, samplingRate);

    // Resampling Right Channel
    if (stereoMode && (sfinfo.channels > 1))
    {
      dataR = resample_vector(notResampledBufferR, sfinfo.samplerate, samplingRate);
    }

    if (stereoMode && (sfinfo.channels == 1))
    {
      dataR = dataL;
    }
  }
}
