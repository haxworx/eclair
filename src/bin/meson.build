

executable('eclair', 'main.c',
  link_with: eclair_core,
  dependencies: deps,
  include_directories: [ inc_dir, inc_dir_lib ],
  install: true
)

# Developer tool: renders any theme directly from its compiled vector layout.

src_tool = files([
  'theme-preview.c',
  meson.project_source_root() / 'src/lib/eclair_turntable.c'
])

executable('theme-preview', src_tool,
  include_directories: inc_dir_lib,
  dependencies: deps
)