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

#ifndef MATHFUNCTIONS_H
#define MATHFUNCTIONS_H

#include <QVector>

struct s_fftw_complex
{
  double real;
  double imagine;
};

void frequency_response_to_impulse_response(double w_in[],
                                            double A_in[],
                                            int n_count,
                                            float IR[],
                                            int IR_n_count,
                                            int rate);

void fft_convolver(float signal[], int signal_n_count, float impulse_response[], int ir_n_count);

void fft_deconvolver(float signal_a[],
                     int signal_a_n_count,
                     float signal_c[],
                     int signal_c_n_count,
                     float impulse_response[],
                     int ir_n_count,
                     float lowcut_relative_frequency,
                     float highcut_relative_frequency
                    );

enum FFT_AVERAGE_TYPE {FFT_AVERAGE_MEAN, FFT_AVERAGE_MAX};

void fft_average(double *buffer,
                 double *avrage_spectrum,
                 int n_spectrum,
                 int n_samples,
                 FFT_AVERAGE_TYPE type);

void calulate_autoeq_amplitude_response(int n_spectrum,
                                        int sample_rate,
                                        double *current_signal,
                                        int n_current_samples,
                                        double *ref_signal,
                                        int n_ref_samples,
                                        double *f_log_values,
                                        double *db_values,
                                        int n_autoeq_points
                                       );

void generate_logarithmic_sweep(double length_sec,
                                int sample_rate,
                                double f_start,
                                double f_end,
                                double sweep_amplitude,
                                float data[]);

QVector<float> resample_vector(QVector<float> sourceBuffer,
                               float sourceSamplerate,
                               float targetSamplerate);
#endif //MATHFUNCTIONS_H
