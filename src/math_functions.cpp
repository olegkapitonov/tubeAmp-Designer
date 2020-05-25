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

#include <QScopedPointer>
#include <gsl/gsl_complex_math.h>
#include <gsl/gsl_multifit.h>
#include <gsl/gsl_spline.h>

#include <fftw3.h>
#include <cstring>

#include "math_functions.h"

#include <zita-resampler/resampler.h>

// Function converts amplitude-frequency response
// to time domain impulse response.
void frequency_response_to_impulse_response(double w_in[],
                                            double A_in[],
                                            int n_count,
                                            float IR[],
                                            int IR_n_count,
                                            int rate)
{
  // Additional high frequency point.
  // Needed for better interpolation
  // at high frequency edge of the input response
  double far_point = A_in[n_count - 1]/100.0;
  double far_point_w = w_in[n_count - 1]*10.0;

  n_count++;

  // Convert frequency response to symmetrical form
  // (with negative frequencies)
  // [0, w_max] => [-w_max, w_max]
  QVector<double> A_sym(n_count * 2);
  QVector<double> w_sym(n_count * 2);

  // Mirror positive frequency values
  // to negative frequency values
  A_sym[n_count*2 - 1] = log(far_point);
  A_sym[0] = log(far_point);
  w_sym[n_count*2 - 1] = log10(far_point_w);
  w_sym[0] = -log10(far_point_w);

  for (int i = 1; i < n_count; i++)
  {
    A_sym[i + n_count - 1] = log(A_in[i - 1]);
    A_sym[-i - 1 + n_count + 1] = log(A_in[i - 1]);
    w_sym[i + n_count - 1] = log10(w_in[i - 1]);
    w_sym[-i - 1 + n_count + 1] = -log10(w_in[i - 1]);
  }

  double specrum_freq_step = rate / IR_n_count;
  // To perform this conversion, we must calculate
  // minimum phase response by Hilbert transform
  // of log(A(w))
  // hilbert_count is count of frequency points in interpolated spectrum
  // which will be used as input for Hilbert transform
  int hilbert_count = 2 * floor(far_point_w / 2.0 / M_PI / specrum_freq_step);

  // Use GSL spline interpolator
  gsl_interp_accel *acc = gsl_interp_accel_alloc ();
  gsl_spline *spline = gsl_spline_alloc (gsl_interp_cspline, n_count*2);

  gsl_spline_init (spline, w_sym.data(), A_sym.data(), n_count*2);

  // Interpolated log(A(w)) response
  QVector<double> Alog(hilbert_count);
  QVector<double> ww(hilbert_count);

  // Perform interpolation
  for (int i = 0; i < hilbert_count; i++)
  {
    ww[i] = 2.0*M_PI*(-specrum_freq_step * hilbert_count / 2.0 +
      specrum_freq_step * hilbert_count / 2.0 *2.0 / (double)(hilbert_count) * i);

    if (ww[i] < 0)
    {
      Alog[i] = gsl_spline_eval(spline, -log10(-ww[i]), acc);
    }
    else if (ww[i] != 0)
    {
      Alog[i] = gsl_spline_eval(spline, log10(ww[i]), acc);
    }

    if (ww[i] == 0.0)
    {
      Alog[i] = gsl_spline_eval(spline, 0.0, acc);
    }
  }

  gsl_spline_free (spline);
  gsl_interp_accel_free (acc);

  // Frequency response
  QVector<double> F(hilbert_count);

  // Perform Hilbert transform in frequency domain.
  // For this:
  // 1. Perform FFT of input interpolated log(A(w)) response
  QVector<s_fftw_complex> out(hilbert_count/2+1);

  fftw_plan p,p1;

  p = fftw_plan_dft_r2c_1d(hilbert_count, Alog.data(),
    (double (*)[2])out.data(), FFTW_ESTIMATE);

  fftw_execute(p);

  // 2. Perform Hilbert transform (swap real and imaginary parts)
  out[0].real = 0.0;
  out[0].imagine = 0.0;

  out[hilbert_count/2].real = 0.0;
  out[hilbert_count/2].imagine = 0.0;

  for (int i=1;i<hilbert_count/2;i++)
  {
    double a,b;
    a = out[i].real;
    b = out[i].imagine;
    out[i].imagine = a;
    out[i].real = -b;
  }

  // 3. Perform inverse FFT
  p1 = fftw_plan_dft_c2r_1d(hilbert_count, (double (*)[2])out.data(),
    F.data(), FFTW_ESTIMATE);

  fftw_execute(p1);

  fftw_destroy_plan(p);
  fftw_destroy_plan(p1);

  // 4. Normalize result, get minimum phase response (F[])
  QVector<double> F_interp(hilbert_count/2);

  for (int i=hilbert_count/2;i<hilbert_count;i++)
  {
    F_interp[i-hilbert_count/2] = F[i] / hilbert_count; // !!!!!!!!!!!!!!!!!!!!!!!!
  }

  // Construct spectrum in complex form
  // from amplitude and phase responses
  int spectrum_count = IR_n_count / 2 + 1;

  QVector<s_fftw_complex> spectrum(spectrum_count);

  for (int i=1; i<spectrum_count;i++)
  {
    gsl_complex A = gsl_complex_polar(exp(Alog[i+hilbert_count/2]), F_interp[i]);
    spectrum[i].real = GSL_REAL(A);
    spectrum[i].imagine = GSL_IMAG(A);
  }

  // Kill constant component
  spectrum[0].real = 0.0;
  spectrum[0].imagine = 0.0;

  // Get output impulse response from spectrum by inverse FFT
  QVector<double> IR_internal(IR_n_count);

  p1 = fftw_plan_dft_c2r_1d(IR_n_count, (double (*)[2])spectrum.data(),
    IR_internal.data(), FFTW_ESTIMATE);

  fftw_execute(p1);

  fftw_destroy_plan(p1);

  // Calculated frequency response is not accurate.
  // This may lead to problems in impulse response -
  // it will start at time t < 0 instead of t = 0
  // We have no negative time in IR_internal buffer,
  // so n first samples of the responce will be placed
  // at the end of the response (inverse FFT has that effect).

  // We must return this samples back to the beginning
  // by circular rotating of the buffer.
  // For this:
  // 1. Find maximum sample value
  double max_sample = 0.0;

  for (int i = 0; i < IR_n_count; i++)
  {
    if (fabs(IR_internal[i]) > max_sample)
    {
      max_sample = fabs(IR_internal[i]);
    }
  }

  // 2. Find number of sample at which impulse response starts.
  // At this sample RMS amplitude will jump sharply
  // to the value near max_sample
  int impulse_start_sample = 0;

  for (int i = (IR_n_count - 1); i >= 4; i--)
  {
    double rms = sqrt((pow(
      IR_internal[i], 2) +
      pow(IR_internal[i - 1], 2) +
      pow(IR_internal[i - 2], 2) +
      pow(IR_internal[i - 3], 2) +
      pow(IR_internal[i - 4], 2)
    ) / 5.0);

    if (rms < max_sample / 100.0)
    {
      impulse_start_sample = i;
      break;
    }
  }

  // 3. Perform circular rotation and normalization
  for (int i = impulse_start_sample; i < IR_n_count; i++)
  {
    IR[i - impulse_start_sample] = IR_internal[i] / IR_n_count;
  }

  for (int i = 0; i < impulse_start_sample; i++)
  {
    IR[i + IR_n_count - impulse_start_sample] = IR_internal[i] / IR_n_count;
  }
}

// Calculates convolution of signal and impulse response
// in frequency domain
void fft_convolver(float signal[], int signal_n_count, float impulse_response[],
                   int ir_n_count)
{
  int n_count;

  // Signal and impulse responce must have the same length,
  // so we choose maximum length from both and extend
  // shorter impulse response (or signal)
  if (signal_n_count >= ir_n_count)
  {
    n_count = signal_n_count;
  }
  else
  {
    n_count = ir_n_count;
  }

  QVector<double> signal_double(n_count);

  for (int i = 0; i < signal_n_count; i++)
  {
    signal_double[i] = signal[i];
  }

  for (int i = signal_n_count; i < n_count; i++)
  {
    signal_double[i] = 0.0;
  }

  // Get spectrum of the signal
  QVector<s_fftw_complex> signal_spectrum(n_count / 2 + 1);

  fftw_plan p;

  p = fftw_plan_dft_r2c_1d(n_count, signal_double.data(),
    (double (*)[2])signal_spectrum.data(), FFTW_ESTIMATE);

  fftw_execute(p);
  fftw_destroy_plan(p);

  // Extend impulse response to signal length if needed
  QVector<double> impulse_response_double(n_count);

  for (int i = 0; i < ir_n_count; i++)
  {
    impulse_response_double[i] = impulse_response[i];
  }

  for (int i = ir_n_count; i < n_count; i++)
  {
    impulse_response_double[i] = 0.0;
  }

  // Get spectrum of the frequency response
  QVector<s_fftw_complex> impulse_response_spectrum(n_count / 2 + 1);

  p = fftw_plan_dft_r2c_1d(n_count, impulse_response_double.data(),
    (double (*)[2])impulse_response_spectrum.data(), FFTW_ESTIMATE);

  fftw_execute(p);
  fftw_destroy_plan(p);

  // Perform convolution in frequency domain
  // result = signal * impulse_response
  for (int i = 0; i < n_count / 2 + 1; i++)
  {
    gsl_complex signal_A = gsl_complex_rect(signal_spectrum[i].real,
      signal_spectrum[i].imagine);
    gsl_complex impulse_response_A = gsl_complex_rect(impulse_response_spectrum[i].real,
      impulse_response_spectrum[i].imagine);

    gsl_complex result_A = gsl_complex_mul(signal_A, impulse_response_A);

    signal_spectrum[i].real = GSL_REAL(result_A);
    signal_spectrum[i].imagine = GSL_IMAG(result_A);
  }

  // Perform inverse FFT, get output signal
  p = fftw_plan_dft_c2r_1d(n_count, (double (*)[2])signal_spectrum.data(),
    signal_double.data(), FFTW_ESTIMATE);

  fftw_execute(p);
  fftw_destroy_plan(p);

  // Normalize output signal
  for (int i = 0; i < signal_n_count; i++)
  {
    signal[i] = signal_double[i] / n_count;
  }
}

// Recreates impulse response
// from test signal (signal_a)
// and response signal (signal_c)
// in frequency domain.
// Filters result by lowpass and hipass
void fft_deconvolver(float signal_a[],
                     int signal_a_n_count,
                     float signal_c[],
                     int signal_c_n_count,
                     float impulse_response[],
                     int ir_n_count,
                     float lowcut_relative_frequency,
                     float highcut_relative_frequency
                    )
{
  int n_count = signal_c_n_count;
  QVector<double> signal_c_double(n_count);

  for (int i = 0; i < n_count; i++)
  {
    signal_c_double[i] = signal_c[i];
  }

  // Calculate response signal spectrum
  QVector<s_fftw_complex> signal_c_spectrum(n_count / 2 + 1);

  fftw_plan p;

  p = fftw_plan_dft_r2c_1d(n_count, signal_c_double.data(),
    (double (*)[2])signal_c_spectrum.data(), FFTW_ESTIMATE);

  fftw_execute(p);
  fftw_destroy_plan(p);

  QVector<double> signal_a_double(n_count);

  for (int i = 0; i < signal_a_n_count; i++)
  {
    signal_a_double[i] = signal_a[i];
  }

  for (int i = signal_a_n_count; i < n_count; i++)
  {
    signal_a_double[i] = 0.0;
  }

  // Calculate test signal frequency
  QVector<s_fftw_complex> signal_a_spectrum(n_count / 2 + 1);

  p = fftw_plan_dft_r2c_1d(n_count, signal_a_double.data(),
    (double (*)[2])signal_a_spectrum.data(), FFTW_ESTIMATE);

  fftw_execute(p);
  fftw_destroy_plan(p);

  QVector<double> impulse_response_double(n_count);
  QVector<s_fftw_complex> impulse_response_spectrum(n_count / 2 + 1);

  // Perform deconvolution in frequency domain
  // impulse_response = signal_c / signal_a
  for (int i = 0; i < n_count / 2 + 1; i++)
  {
    gsl_complex signal_a_A = gsl_complex_rect(signal_a_spectrum[i].real,
                                              signal_a_spectrum[i].imagine);
    gsl_complex signal_c_A = gsl_complex_rect(signal_c_spectrum[i].real,
                                              signal_c_spectrum[i].imagine);

    gsl_complex impulse_response_A = gsl_complex_div(signal_c_A, signal_a_A);

    impulse_response_spectrum[i].real = GSL_REAL(impulse_response_A);
    impulse_response_spectrum[i].imagine = GSL_IMAG(impulse_response_A);
  }

  // Kill constant component
  impulse_response_spectrum[0].real = 0.0;
  impulse_response_spectrum[0].imagine = 0.0;

  // Perform lowpass and hipass filtering
  for (int i = 1; i < impulse_response_spectrum.size(); i++)
  {
    double lowcut_T = 1.0 / (2.0 * M_PI * lowcut_relative_frequency);
    double highcut_T = 1.0 / (2.0 * M_PI * highcut_relative_frequency);

    gsl_complex jw = gsl_complex_rect(0.0, 2 * M_PI * 0.5 * (double)i /
      impulse_response_spectrum.size());
    gsl_complex highcut_A = gsl_complex_div(gsl_complex_rect(1.0, 0.0),
      gsl_complex_add(gsl_complex_mul(jw, gsl_complex_rect(highcut_T, 0.0)),
                      gsl_complex_rect(1.0,0.0))
    );

    gsl_complex lowcut_A = gsl_complex_div(gsl_complex_mul(gsl_complex_rect(lowcut_T, 0.0),
      jw), gsl_complex_add(gsl_complex_mul(jw, gsl_complex_rect(lowcut_T, 0.0)),
                      gsl_complex_rect(1.0,0.0))
    );

    gsl_complex freq = gsl_complex_rect(impulse_response_spectrum[i].real,
                                        impulse_response_spectrum[i].imagine);

    for (int j = 0; j < 3; j++)
    {
      freq = gsl_complex_mul(freq, lowcut_A);
      freq = gsl_complex_mul(freq, highcut_A);
    }

    impulse_response_spectrum[i].real = GSL_REAL(freq);
    impulse_response_spectrum[i].imagine = GSL_IMAG(freq);
  }

  // Perform inverse FFT, get impulse response
  p = fftw_plan_dft_c2r_1d(n_count, (double (*)[2])impulse_response_spectrum.data(),
    impulse_response_double.data(), FFTW_ESTIMATE);

  fftw_execute(p);
  fftw_destroy_plan(p);

  // Normalize impulse response
  for (int i = 0; i < ir_n_count; i++)
  {
    impulse_response[i] = impulse_response_double[i] / n_count;
  }
}

// Calculates average amplitude value of each
// frequency component of the signal in buffer
// Spectrum calculated on each n_spectrum samples
// type:
// FFT_AVERAGE_MAX - to get maximum value
// FFT_AVERAGE_MEAN - to get mean value
void fft_average(double *average_spectrum,
                 double *buffer,
                 int n_spectrum,
                 int n_samples,
                 FFT_AVERAGE_TYPE type)
{
  QVector<s_fftw_complex> out(n_spectrum + 1);
  memset(average_spectrum, 0.0, n_spectrum * sizeof(double));

  int p_buffer = 0;

  // Calculate spectrum on n_spectrum slice,
  // calculate average value of each frequency in spectrum
  while ((p_buffer + n_spectrum * 2) < n_samples)
  {
    // Perform FFT
    fftw_plan p;

    p = fftw_plan_dft_r2c_1d(n_spectrum * 2, buffer + p_buffer,
      (double (*)[2])out.data(), FFTW_ESTIMATE);

    p_buffer += n_spectrum * 2;

    fftw_execute(p);
    fftw_destroy_plan(p);

    for (int i = 1; i < n_spectrum + 1; i++)
    {
      double A = sqrt(pow(out[i].real, 2) + pow(out[i].imagine, 2)) / n_spectrum / 2.0;
      switch (type)
      {
        case FFT_AVERAGE_MAX:
          if (A > average_spectrum[i - 1])
          {
            average_spectrum[i - 1] = A;
          }
        break;
        case FFT_AVERAGE_MEAN:
          average_spectrum[i - 1] += A / n_spectrum;
        break;
      }
    }
  }
}

// Calculates correction frequency response
// for auto-equalizer
void calulate_autoeq_amplitude_response(int n_spectrum,
                                        int sample_rate,
                                        double *current_signal,
                                        int n_current_samples,
                                        double *ref_signal,
                                        int n_ref_samples,
                                        double *f_log_values,
                                        double *db_values,
                                        int n_autoeq_points
                                       )
{
  QVector<double> current_spectrum(n_spectrum);
  QVector<double> ref_spectrum(n_spectrum);

  // Calculate average spectrums of current signal
  // and reference signal
  fft_average(current_spectrum.data(), current_signal, n_spectrum,
    n_current_samples, FFT_AVERAGE_MAX);

  fft_average(ref_spectrum.data(), ref_signal, n_spectrum,
    n_ref_samples, FFT_AVERAGE_MAX);

  // Calculate diff between spectrums
  QVector<double> diff_spectrum(n_spectrum + 1);
  QVector<double> diff_spectrum_log_freqs(n_spectrum + 1);

  for (int i = 1; i < n_spectrum + 1; i++)
  {
    diff_spectrum[i] = 20.0 * log10(ref_spectrum[i - 1] / current_spectrum[i - 1]);
    diff_spectrum_log_freqs[i] = log10(
      (double)(i + 1)
      / (double)(n_spectrum)
      * sample_rate / 2
    );
  }

  diff_spectrum_log_freqs[0] = 0.0;
  diff_spectrum[0] = diff_spectrum[1];

  gsl_interp_accel *acc = gsl_interp_accel_alloc ();
  gsl_spline *spline = gsl_spline_alloc (gsl_interp_cspline,
                                         n_spectrum + 1);

  gsl_spline_init (spline,
                   diff_spectrum_log_freqs.data(),
                   diff_spectrum.data(),
                   n_spectrum + 1);

  // Interpolate diff spectrum to given frequency values
  QVector<double> interpolated_diff_spectrum_log_freqs(n_autoeq_points * 100);
  QVector<double> interpolated_diff_spectrum(n_autoeq_points * 100);

  for (int i = 0; i < n_autoeq_points * 100; i++)
  {
    interpolated_diff_spectrum_log_freqs[i] = (log10(20000.0) - log10(10.0))
      * (double)(i) / (n_autoeq_points * 100 - 1) + log10(10.0);

    interpolated_diff_spectrum[i] =
      gsl_spline_eval(spline,
                      interpolated_diff_spectrum_log_freqs[i],
                      acc);

  }

  int begin_f_pos = 0;

  for (int i = 0; i < n_autoeq_points - 1; i++)
  {
    double amplitude_sum = 0.0;
    int prev_f_pos = begin_f_pos;

    while (interpolated_diff_spectrum_log_freqs[begin_f_pos] < f_log_values[i])
    {
      amplitude_sum += interpolated_diff_spectrum[begin_f_pos];
      begin_f_pos++;
    }

    int end_f_pos = begin_f_pos;

    while (interpolated_diff_spectrum_log_freqs[end_f_pos] < f_log_values[i + 1])
    {
      amplitude_sum += interpolated_diff_spectrum[end_f_pos];
      end_f_pos++;
    }

    amplitude_sum /= end_f_pos - prev_f_pos;

    db_values[i] = amplitude_sum;
  }

  db_values[n_autoeq_points - 1] =
    db_values[n_autoeq_points - 2];

  // Normalize amplitude response
  double max_amplitude = -DBL_MAX;
  for (int i = 0; i < n_autoeq_points; i++)
  {
    if (max_amplitude < db_values[i])
    {
      max_amplitude = db_values[i];
    }
  }

  for (int i = 0; i < n_autoeq_points; i++)
  {
    db_values[i] -= max_amplitude;
  }
}

// Generates logarithmic sweep signal
// It can be used as a test signal to get inpulse response
void generate_logarithmic_sweep(double length_sec,
                                int sample_rate,
                                double f_start,
                                double f_end,
                                double sweep_amplitude,
                                float data[])
{
  for (int i = 0; i < sample_rate * length_sec; i++)
  {
    float frame_data = sweep_amplitude * sin( 2 * M_PI * length_sec * f_start * (pow(f_end / f_start, (double)i / sample_rate / length_sec) - 1.0) / log(f_end / f_start));
    data[i] = frame_data;
  }
}

// Resamples signal in buffer
QVector<float> resample_vector(QVector<float> sourceBuffer,
                               float sourceSamplerate,
                               float targetSamplerate)
{
  QVector<float> targetBuffer;

  if (sourceSamplerate == targetSamplerate)
  {
    targetBuffer = sourceBuffer;
  }
  else
  {
    float ratio = targetSamplerate/(float)sourceSamplerate;

    targetBuffer.resize(sourceBuffer.size() * ratio);

    QScopedPointer<Resampler> resampl(new Resampler());
    resampl->setup(sourceSamplerate, targetSamplerate, 1, 48);

    int k = resampl->inpsize();

    QVector<float> signalIn(sourceBuffer.size() + k/2 - 1 + k - 1);
    QVector<float> signalOut((int)((sourceBuffer.size() + k/2 - 1 + k - 1) * ratio));

    // Create paddig before and after signal, needed for zita-resampler
    for (int i = 0; i < sourceBuffer.size() + k/2 - 1 + k - 1; i++)
    {
      signalIn[i] = 0.0;
    }

    for (int i = k/2 - 1; i < sourceBuffer.size() + k/2 - 1; i++)
    {
      signalIn[i] = sourceBuffer[i - k/2 + 1];
    }

    resampl->inp_count = sourceBuffer.size() + k/2 - 1 + k - 1;
    resampl->out_count = (sourceBuffer.size() + k/2 - 1 + k - 1) * ratio;
    resampl->inp_data = signalIn.data();
    resampl->out_data = signalOut.data();

    resampl->process();

    for (int i = 0; i < targetBuffer.size(); i++)
    {
      targetBuffer[i] = signalOut[i] / ratio;
    }
  }

  return targetBuffer;
}
