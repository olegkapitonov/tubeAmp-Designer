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

#include <QSharedPointer>
#include <QFile>

#include <gsl/gsl_spline.h>
#include <sndfile.h>

#include "processor.h"
#include "kpp_tubeamp_dsp.h"
#include "float.h"
#include "math_functions.h"

Processor::Processor(int SR)
{
  samplingRate = SR;

  preampCorrectionEnabled = false;
  cabinetCorrectionEnabled = false;

  new_preamp_convproc = nullptr;
  new_preamp_correction_convproc = nullptr;
  new_convproc = nullptr;
  new_correction_convproc = nullptr;

  preamp_convproc = nullptr;
  preamp_correction_convproc = nullptr;
  convproc = nullptr;
  correction_convproc = nullptr;

  dsp = new mydsp();
  dsp->profile = nullptr;

  //convolverDeleteThread = new ConvolverDeleteThread();
}

Processor::~Processor()
{
  cleanProfile();
}

void Processor::cleanProfile()
{
  /*delete preamp_convproc;
  delete preamp_correction_convproc;
  delete convproc;
  delete correction_convproc;

  delete new_preamp_convproc;
  delete new_preamp_correction_convproc;
  delete new_convproc;
  delete new_correction_convproc;*/

  delete dsp->profile;
  delete dsp;

  new_preamp_convproc = nullptr;
  new_preamp_correction_convproc = nullptr;
  new_convproc = nullptr;
  new_correction_convproc = nullptr;
}

// Check *.tapf file signature
int Processor::checkProfileFile(const char *path)
{
  int status = 0;

  QFile profile_file(path);
  profile_file.open(QIODevice::ReadOnly);

  if (profile_file.exists())
  {
    st_profile check_profile;
    if (profile_file.read((char*)&check_profile, sizeof(st_profile)) == sizeof(st_profile))
    {
      if (!strncmp(check_profile.signature, "TaPf", 4))
      {
        status = 1;
      }
    }
    else status = 1;

    profile_file.close();
  }

  return status;
}

bool Processor::loadProfile(QString filename)
{
  if (!checkProfileFile(filename.toLocal8Bit().constData()))
  {
    return false;
  }

  cleanProfile();

  dsp = new mydsp();
  dsp->init(samplingRate);

  dsp->controls.volume = 1.0;
  dsp->controls.drive = 50.0;
  dsp->controls.low = 0.0;
  dsp->controls.middle = 0.0;
  dsp->controls.high = 0.0;
  dsp->controls.mastergain = 100.0;

  profileFileName = filename;
  dsp->profile = new st_profile;

  QFile profile_file(filename);
  profile_file.open(QIODevice::ReadOnly);
  if (profile_file.exists())
  {
    currentProfileFile.clear();
    currentProfileFile.append(filename);

    if (profile_file.read((char*)dsp->profile, sizeof(st_profile)) == sizeof(st_profile))
    {
      QVector<float> preamp_temp_buffer;

      QVector<float> left_temp_buffer;
      QVector<float> right_temp_buffer;

      // IRs in *.tapf are 48000 Hz,
      // calculate ratio for resampling
      float ratio = (float)samplingRate/48000.0;

      st_impulse preamp_impheader, impheader;

      // Load preamp IR data to temp buffer
      if (profile_file.read((char*)&preamp_impheader, sizeof(st_impulse)) != sizeof(st_impulse))
      {
        return false;
      }

      preamp_temp_buffer.resize(preamp_impheader.sample_count);

      if ((quint64)profile_file.read((char*)preamp_temp_buffer.data(),
        sizeof(float) * preamp_impheader.sample_count) !=
        (sizeof(float) * preamp_impheader.sample_count))
      {
        return false;
      }

      // Load cabsym IR data to temp buffers
      for (int i=0;i<2;i++)
      {
        if (profile_file.read((char*)&impheader,
          sizeof(st_impulse)) != sizeof(st_impulse))
        {
          return false;
        }

        if (impheader.channel==0)
        {
          left_temp_buffer.resize(impheader.sample_count);
          if ((quint64)profile_file.read((char*)left_temp_buffer.data(),
            sizeof(float) * impheader.sample_count) !=
            sizeof(float) * impheader.sample_count)
          {
            return false;
          }
        }
        if (impheader.channel==1)
        {
          right_temp_buffer.resize(impheader.sample_count);
          if ((quint64)profile_file.read((char*)right_temp_buffer.data(),
            sizeof(float) * impheader.sample_count) !=
            sizeof(float) * impheader.sample_count)
          {
            return false;
          }
        }
      }
      preamp_impulse = resample_vector(preamp_temp_buffer, 48000, samplingRate);
      left_impulse = resample_vector(left_temp_buffer, 48000, samplingRate);
      right_impulse = resample_vector(right_temp_buffer, 48000, samplingRate);

      preamp_correction_impulse.resize(preamp_impheader.sample_count * ratio);
      left_correction_impulse.resize(impheader.sample_count * ratio);
      right_correction_impulse.resize(impheader.sample_count * ratio);

      for (int i = 0; i < preamp_impheader.sample_count*ratio; i++)
      {
        preamp_correction_impulse[i] = 0.0f;
      }

      preamp_correction_impulse[0] = 1.0f;

      for (int i = 0; i < impheader.sample_count * ratio; i++)
      {
        left_correction_impulse[i] = 0.0f;
        right_correction_impulse[i] = 0.0f;
      }

      left_correction_impulse[0] = 1.0f;
      right_correction_impulse[0] = 1.0f;

      // Create preamp convolver
      preamp_convproc = createMonoConvolver(preamp_impulse);

      // Create cabsym convolver
      convproc = createStereoConvolver(left_impulse, right_impulse);

      // Create preamp correction convolver
      preamp_correction_convproc = createMonoConvolver(preamp_correction_impulse);

      // Create cabsym correction convolver
      correction_convproc = createStereoConvolver(left_correction_impulse,
                                                  right_correction_impulse);

      profile_file.close();
    }
  }
  else
  {
    return false;
  }

  return true;
}

bool Processor::saveProfile(QString filename)
{
  FILE * profile_file= fopen(filename.toLocal8Bit().constData(), "wb");
  if (profile_file != NULL)
  {
    QVector<float> savePreampImpulse;
    QVector<float> saveLeftImpulse;
    QVector<float> saveRightImpulse;

    savePreampImpulse = resample_vector(preamp_impulse, samplingRate, 48000);
    saveLeftImpulse = resample_vector(left_impulse, samplingRate, 48000);
    saveRightImpulse = resample_vector(right_impulse, samplingRate, 48000);

    st_impulse impulse_preamp_header, impulse_left_header, impulse_right_header;

    impulse_preamp_header.sample_rate = 48000;
    impulse_preamp_header.channel = 0;
    impulse_preamp_header.sample_count = savePreampImpulse.size();

    impulse_left_header.sample_rate = 48000;
    impulse_left_header.channel = 0;
    impulse_left_header.sample_count = saveLeftImpulse.size();

    impulse_right_header.sample_rate = 48000;
    impulse_right_header.channel = 1;
    impulse_right_header.sample_count = saveRightImpulse.size();

    fwrite(dsp->profile, sizeof(st_profile), 1, profile_file);

    fwrite(&impulse_preamp_header, sizeof(st_impulse), 1, profile_file);
    fwrite(savePreampImpulse.data(), sizeof(float), savePreampImpulse.size(),
           profile_file);

    fwrite(&impulse_left_header, sizeof(st_impulse), 1, profile_file);
    fwrite(saveLeftImpulse.data(), sizeof(float), saveLeftImpulse.size(), profile_file);

    fwrite(&impulse_right_header, sizeof(st_impulse), 1, profile_file);
    fwrite(saveRightImpulse.data(), sizeof(float), saveRightImpulse.size(),
           profile_file);

    fclose(profile_file);

    return true;
  }

  return false;
}

QVector<float> Processor::getFrequencyResponse(QVector<float> freqs,
                                               QVector<float> impulse)
{
  QVector<float> frequencyResponse(freqs.size());

  QVector<double> double_impulse(impulse.size());

  for (int i = 0; i < impulse.size(); i++)
  {
    double_impulse[i] = impulse[i];
  }

  fftw_complex out[impulse.size() / 2 + 1];
  fftw_plan p;

  p = fftw_plan_dft_r2c_1d(impulse.size(), double_impulse.data(), out, FFTW_ESTIMATE);

  fftw_execute(p);
  fftw_destroy_plan(p);

  QVector<double> rawFrequencyResponse(impulse.size() / 2);
  QVector<double> rawFreqs(impulse.size() / 2);

  for (int i = 0; i < rawFrequencyResponse.size(); i++)
  {
    rawFrequencyResponse[i] = sqrt(pow(out[i + 1][0], 2) + pow(out[i + 1][1], 2));
    rawFreqs[i] = ((double)(i + 1) / rawFrequencyResponse.size()) * (samplingRate / 2);
  }

  gsl_interp_accel *acc = gsl_interp_accel_alloc ();
  gsl_spline *spline = gsl_spline_alloc (gsl_interp_cspline, rawFreqs.size());

  gsl_spline_init (spline, rawFreqs.data(), rawFrequencyResponse.data(),
                   rawFreqs.size());

  float maxAmplitude = FLT_MIN;

  for (int i = 0; i < freqs.size(); i++)
  {
    frequencyResponse[i] = gsl_spline_eval(spline, freqs[i], acc);
    if (frequencyResponse[i] > maxAmplitude)
    {
      maxAmplitude = frequencyResponse[i];
    }
  }

  for (int i = 0; i < freqs.size(); i++)
  {
    frequencyResponse[i] /= maxAmplitude;
  }

  gsl_spline_free(spline);
  gsl_interp_accel_free(acc);

  return frequencyResponse;
}

QVector<float> Processor::getPreampFrequencyResponse(QVector<float> freqs)
{
  return getFrequencyResponse(freqs, preamp_impulse);
}

QVector<float> Processor::getCabinetSumFrequencyResponse(QVector<float> freqs)
{
  QVector<float> leftResponse = getFrequencyResponse(freqs, left_impulse);
  QVector<float> rightResponse = getFrequencyResponse(freqs, right_impulse);

  QVector<float> sumResponse(leftResponse.size());

  for (int i = 0; i < sumResponse.size(); i++)
  {
    sumResponse[i] = (leftResponse[i] + rightResponse[i]) / 2.0;
  }

  return sumResponse;
}

stControls Processor::getControls()
{
  return dsp->controls;
}

void Processor::setControls(stControls newControls)
{
  dsp->controls = newControls;
}

st_profile Processor::getProfile()
{
  return *(dsp->profile);
}

void Processor::setProfile(st_profile newProfile)
{
  *(dsp->profile) = newProfile;
}

float hardClipBottom(float input, float cut)
{
  if (input < cut) input = cut;

  return input;
}

float Ks(float input, float Upor, float Kreg)
{
  return 1.0 / (hardClipBottom((input - Upor) * Kreg, 0) + 1);
}

float Ksplus(float input, float Upor)
{
  return Upor - input * Upor;
}

float Processor::tube(float Uin, float Kreg, float Upor, float bias, float cut)
{
  // Model of tube nonlinear distortion
  return hardClipBottom(Uin * Ks(Uin, Upor, Kreg)
    + Ksplus(Ks(Uin, Upor, Kreg), Upor) + bias, cut);
}

void Processor::setPreampCorrectionImpulseFromFrequencyResponse(QVector<double> w,
                                                                QVector<double> A)
{
  preamp_correction_impulse.resize(preamp_impulse.size());

  frequency_response_to_impulse_response(w.data(),
                                         A.data(),
                                         w.size(),
                                         preamp_correction_impulse.data(),
                                         preamp_correction_impulse.size(),
                                         samplingRate);

  if (new_preamp_correction_convproc == nullptr)
  {
    new_preamp_correction_convproc = createMonoConvolver(preamp_correction_impulse);
  }

  preampCorrectionEnabled = true;
}

void Processor::setCabinetSumCorrectionImpulseFromFrequencyResponse(QVector<double> w,
                                                                    QVector<double> A)
{
  left_correction_impulse.resize(left_impulse.size());

  frequency_response_to_impulse_response(w.data(),
                                         A.data(),
                                         w.size(),
                                         left_correction_impulse.data(),
                                         left_correction_impulse.size(),
                                         samplingRate);

  right_correction_impulse.resize(right_impulse.size());

  frequency_response_to_impulse_response(w.data(),
                                         A.data(),
                                         w.size(),
                                         right_correction_impulse.data(),
                                         right_correction_impulse.size(),
                                         samplingRate);

  if (new_correction_convproc == nullptr)
  {
    new_correction_convproc = createStereoConvolver(left_correction_impulse,
                                                    right_correction_impulse);
  }

  cabinetCorrectionEnabled = true;
}

void Processor::applyPreampCorrection()
{
  fft_convolver(preamp_impulse.data(),
                preamp_impulse.size(),
                preamp_correction_impulse.data(),
                preamp_correction_impulse.size());

  if (new_preamp_convproc == nullptr)
  {
    new_preamp_convproc = createMonoConvolver(preamp_impulse);
  }
}

void Processor::applyCabinetSumCorrection()
{
  fft_convolver(left_impulse.data(), left_impulse.size(),
                left_correction_impulse.data(), left_correction_impulse.size());

  fft_convolver(right_impulse.data(), right_impulse.size(),
                right_correction_impulse.data(), right_correction_impulse.size());

  if (new_convproc == nullptr)
  {
    new_convproc = createStereoConvolver(left_impulse, right_impulse);
  }
}

void Processor::resetPreampCorrection()
{
  for (int i = 0; i < preamp_correction_impulse.size(); i++)
  {
    preamp_correction_impulse[i] = 0.0f;
  }

  preamp_correction_impulse[0] = 1.0f;

  if (new_preamp_correction_convproc == nullptr)
  {
    new_preamp_correction_convproc = createMonoConvolver(preamp_correction_impulse);
  }

  preampCorrectionEnabled = false;
}

void Processor::resetCabinetSumCorrection()
{
  for (int i = 0; i < left_correction_impulse.size(); i++)
  {
    left_correction_impulse[i] = 0.0f;
  }

  left_correction_impulse[0] = 1.0f;

  for (int i = 0; i < right_correction_impulse.size(); i++)
  {
    right_correction_impulse[i] = 0.0f;
  }

  right_correction_impulse[0] = 1.0f;

  if (new_correction_convproc == nullptr)
  {
    new_correction_convproc = createStereoConvolver(left_correction_impulse,
      right_correction_impulse);
  }

  cabinetCorrectionEnabled = false;
}

int Processor::getSamplingRate()
{
  return samplingRate;
}

void Processor::process(float *outL, float *outR, float *in, int nSamples)
{
  // Change convolvers if new available

  if (new_preamp_convproc != nullptr)
  {
    //freeConvolver(preamp_convproc);
    preamp_convproc = new_preamp_convproc;
    new_preamp_convproc = nullptr;
  }

  if (new_preamp_correction_convproc != nullptr)
  {
    //freeConvolver(preamp_correction_convproc);
    preamp_correction_convproc = new_preamp_correction_convproc;
    new_preamp_correction_convproc = nullptr;
  }

  if (new_convproc != nullptr)
  {
    //freeConvolver(convproc);
    convproc = new_convproc;
    new_convproc = nullptr;
  }

  if (new_correction_convproc != nullptr)
  {
    //freeConvolver(correction_convproc);
    correction_convproc = new_correction_convproc;
    new_correction_convproc = nullptr;
  }

  // Preamp convolver

  // Zita-convolver accepts 'fragm' number of samples,
  // real buffer size may be greater,
  // so perform convolution in multiple steps
  int bufp = 0;
  QVector<float> intermediateBuffer(nSamples);

  while (bufp < nSamples)
  {
    memcpy (preamp_convproc->inpdata(0), in + bufp, fragm * sizeof(float));
    preamp_convproc->process (true);
    memcpy (intermediateBuffer.data() + bufp, preamp_convproc->outdata(0),
            fragm * sizeof(float));

    bufp += fragm;
  }

  // Preamp correction convolver
  if (preampCorrectionEnabled)
  {
    bufp = 0;

    while (bufp < nSamples)
    {
      memcpy (preamp_correction_convproc->inpdata(0), intermediateBuffer.data()
        + bufp, fragm * sizeof(float));
      preamp_correction_convproc->process (true);
      memcpy (intermediateBuffer.data() + bufp,
              preamp_correction_convproc->outdata(0), fragm * sizeof(float));

      bufp += fragm;
    }
  }

  // Apply main tubeAmp model from FAUST code
  float *inputs[1] = {intermediateBuffer.data()};
  float *outputs[1] = {outL};

  dsp->compute(nSamples, inputs, outputs);

  memcpy(outR, outL, sizeof(float) * nSamples);

  // Cabinet simulation convolver
  bufp = 0;

  while (bufp < nSamples)
  {
    memcpy (convproc->inpdata(0), outL + bufp, fragm * sizeof(float));
    memcpy (convproc->inpdata(1), outR + bufp, fragm * sizeof(float));

    convproc->process (true);
    memcpy (outL + bufp, convproc->outdata(0), fragm * sizeof(float));
    memcpy (outR + bufp, convproc->outdata(1), fragm * sizeof(float));

    bufp += fragm;
  }

  // Cabinet correction convolver
  if (cabinetCorrectionEnabled)
  {
    bufp = 0;

    while (bufp < nSamples)
    {
      memcpy (correction_convproc->inpdata(0), outL + bufp, fragm * sizeof(float));
      memcpy (correction_convproc->inpdata(1), outR + bufp, fragm * sizeof(float));

      correction_convproc->process (true);
      memcpy (outL + bufp, correction_convproc->outdata(0), fragm * sizeof(float));
      memcpy (outR + bufp, correction_convproc->outdata(1), fragm * sizeof(float));

      bufp += fragm;
    }
  }
}

void Processor::freeConvolver(Convproc *convolver)
{
  auto convolverDeleteThread = new ConvolverDeleteThread();
  QObject::connect(convolverDeleteThread, &QThread::finished,
                   convolverDeleteThread, &QThread::deleteLater);
  convolverDeleteThread->convolver = convolver;
  convolverDeleteThread->start();
}

Convproc* Processor::createMonoConvolver(QVector<float> impulse)
{
  Convproc *newConv = new Convproc;
  newConv->configure (1, 1, impulse.size(),
                      fragm, fragm, Convproc::MAXPART, 0.0);
  newConv->impdata_create (0, 0, 1, impulse.data(),
                           0, impulse.size());

  newConv->start_process(CONVPROC_SCHEDULER_PRIORITY,
                         CONVPROC_SCHEDULER_CLASS);

  return newConv;
}

Convproc* Processor::createStereoConvolver(QVector<float> l_impulse,
                                           QVector<float> r_impulse)
{
  Convproc *newConv = new Convproc;
  newConv->configure(2, 2, l_impulse.size(), fragm, fragm, Convproc::MAXPART, 0.0);

  newConv->impdata_create(0, 0, 1, l_impulse.data(), 0, l_impulse.size());
  newConv->impdata_create(1, 1, 1, r_impulse.data(), 0, r_impulse.size());

  newConv->start_process(CONVPROC_SCHEDULER_PRIORITY, CONVPROC_SCHEDULER_CLASS);

  return newConv;
}

QString Processor::getProfileFileName()
{
  return profileFileName;
}

bool Processor::isPreampCorrectionEnabled()
{
  return preampCorrectionEnabled;
}

bool Processor::isCabinetCorrectionEnabled()
{
  return cabinetCorrectionEnabled;
}

void Processor::setPreampImpulse(QVector<float> data)
{
  preamp_impulse = data;

  if (new_preamp_convproc == nullptr)
  {
    new_preamp_convproc = createMonoConvolver(preamp_impulse);
  }
}

void Processor::setCabinetImpulse(QVector<float> dataL, QVector<float> dataR)
{
  left_impulse = dataL;
  right_impulse = dataR;

  if (new_convproc == nullptr)
  {
    new_convproc = createStereoConvolver(left_impulse, right_impulse);
  }
}

QVector<float> Processor::getPreampImpulse()
{
  return preamp_impulse;
}

QVector<float> Processor::getLeftImpulse()
{
  return left_impulse;
}

QVector<float> Processor::getRightImpulse()
{
  return right_impulse;
}

void Processor::setPreampCorrectionStatus(bool status)
{
  preampCorrectionEnabled = status;
}

void Processor::setCabinetCorrectionStatus(bool status)
{
  cabinetCorrectionEnabled = status;
}

void Processor::setProfileFileName(QString name)
{
  profileFileName = name;
}

void ConvolverDeleteThread::run()
{
  delete convolver;
}
