subdir('images')

themes += custom_target('themes_gant', input: 'gant.edc', output: 'gant.edj',
  depfile: '@BASENAME@.d',
  depend_files: theme_vectors,
  command: [edje_cc, '-deps', '@DEPFILE@', '-id', meson.current_source_dir() / 'images',
    '-fd', meson.project_source_root() / 'data/fonts', '@INPUT@', '@OUTPUT@'],
  install: true, install_dir: data_dir / 'themes')
