themes = []
edje_cc = find_program('edje_cc')
data_dir = get_option('datadir') / 'eclair'
install_data(
  'fonts/Vera.COPYING',
  'fonts/VeraBd.ttf',
  'fonts/fonts.alias',
  'fonts/fonts.dir',
  install_dir: data_dir / 'fonts')

subdir('themes/default')
subdir('themes/gant')
subdir('themes/nocturne')
subdir('themes/folio')
subdir('themes/frequency')
subdir('themes/voltage')
subdir('themes/undertow')

# Desktop launchers resolve Icon=eclair through the hicolor icon theme.
desktop_conf = configuration_data()
desktop_conf.set('ECLAIR_EXECUTABLE', get_option('prefix') / get_option('bindir') / 'eclair')
configure_file(input: 'eclair.desktop', output: 'eclair.desktop',
               configuration: desktop_conf,
               install_dir: get_option('datadir') / 'applications')
install_data('icons/hicolor/scalable/apps/eclair.svg',
             install_dir: get_option('datadir') / 'icons/hicolor/scalable/apps')
foreach size : ['16', '24', '32', '48', '64', '128', '256', '512']
  resolution = size + 'x' + size
  install_data('icons/hicolor' / resolution / 'apps/eclair.png',
               install_dir: get_option('datadir') / 'icons/hicolor' / resolution / 'apps')
endforeach
