project('gst-libopenaptx', 'c',
  version: '0.0.1',
  license: 'LGPL-2.1-only',
)

cdata = configuration_data()
cdata.set_quoted('PACKAGE', meson.project_name())
cdata.set_quoted('VERSION', meson.project_version())
cdata.set_quoted('LICENSE', 'LGPL') # gstreamer rejects the full SPDX identifier
cdata.set_quoted('ORIGIN', 'https://github.com/pali/libopenaptx')
cdata.set_quoted('PACKAGE', 'libopenaptx')

configure_file(output: 'config.h', configuration: cdata)

sources = [
  'plugin.c',
  'encoder.c',
  'decoder.c',
]

gstreamer = dependency('gstreamer-1.0')
gstreamer_audio = dependency('gstreamer-audio-1.0')
libopenaptx = dependency('libopenaptx')

gstreamer_pluginsdir = gstreamer.get_variable(pkgconfig: 'pluginsdir')

library('gstopenaptx', sources,
  dependencies : [gstreamer_audio, libopenaptx],
  install: true, install_dir: gstreamer_pluginsdir
)
