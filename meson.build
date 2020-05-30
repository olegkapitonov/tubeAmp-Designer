project('tubeAmp Designer', 'cpp', version: '1.2', license: ['GPL3'],
  default_options : ['buildtype=release'])

qt5 = import('qt5')

qt5_dep = dependency('qt5', modules: ['Core', 'Gui', 'Widgets'])
gsl_dep = dependency('gsl')
fftw3_dep = dependency('fftw3')
fftw3f_dep = dependency('fftw3f')
jack_dep = dependency('jack')
sndfile_dep = dependency('sndfile')
thread_dep = dependency('threads')

cc = meson.get_compiler('cpp')
zita_convolver_dep = cc.find_library('zita-convolver')
zita_resampler_dep = cc.find_library('zita-resampler')

inc = include_directories('.')

install_data('data/real_test.wav', install_dir : get_option('datadir') / 'tubeAmp Designer')
install_data('tAD.desktop', install_dir : get_option('datadir') / 'applications')
install_data('tAD.png', install_dir : get_option('datadir') / 'pixmaps')
install_subdir('profiles', install_dir : get_option('datadir') / 'tubeAmp Designer')

subdir('FAUST')
subdir('src')


