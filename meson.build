project('eclair', 'c', version: '0.0.1', meson_version: '>=0.61',
        default_options: ['c_std=gnu99', 'warning_level=1'])

cc = meson.get_compiler('c')

deps = []

foreach name : [ 'elementary', 'json-c', 'taglib_c', 'threads' ]
  deps += dependency(name)
endforeach

m_dep = cc.find_library('m', required : false)
if m_dep.found()
  deps += m_dep
endif

conf = configuration_data()
conf.set_quoted('PACKAGE', meson.project_name())
conf.set_quoted('VERSION', meson.project_version())
conf.set_quoted('PACKAGE_DATA_DIR', get_option('prefix') / get_option('datadir') / 'eclair')
conf.set_quoted('PACKAGE_BIN_DIR', get_option('prefix') / get_option('bindir'))
conf.set_quoted('PACKAGE_LIB_DIR', get_option('prefix') / get_option('libdir'))

configure_file(output: 'config.h', configuration: conf)

inc_dir = include_directories('.')

subdir('data')
subdir('src/lib')
subdir('src/bin')
if get_option('tests').enabled()
  subdir('src/tests')
endif

summary(
  {
    'Build Type': get_option('buildtype'),
    'Prefix': get_option('prefix'),
    'Tests': get_option('tests').enabled()
  },
  bool_yn: true,
)
