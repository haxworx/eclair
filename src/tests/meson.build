
# theme load

src = files([
  'theme-load.c'
])

theme_test = executable('theme-load-test', src,
  dependencies: deps
)

test('load all theme collections', theme_test,
  args: themes,
  env: [
    'EINA_LOG_LEVEL=0',
    'ECORE_NO_SYSTEM_MODULES=1'
  ]
)

# scaling

src = files([
  'scaling.c',
  meson.project_source_root() / 'src/lib/eclair_scale.c'
])

scale_test = executable('scaling-test', src,
  include_directories: include_directories('..'),
  dependencies: deps
)

foreach factor : ['1', '1.5', '2']
  test('desktop scale ' + factor, scale_test,
    args: [factor, themes[0]],
    env: [
      'ELM_SCALE=' + factor,
      'EINA_LOG_LEVEL=0',
      'ECORE_NO_SYSTEM_MODULES=1'
    ]
  )
endforeach

# window render

srd = files([
  'window-render.c'
])

window_test = executable('window-render-test', src,
  link_with: eclair_core,
  include_directories: inc_dir_lib,
  dependencies: deps)

foreach factor : ['1', '1.5', '2']
  test('themed window at ' + factor, window_test,
    args: [themes[0]],
    suite: 'display',
    env: [
      'ELM_SCALE=' + factor,
      'EINA_LOG_LEVEL=1',
      'ECORE_EVAS_COMP_NOSYNC=1',
      'EINA_LOG_ABORT=1', 'EINA_LOG_ABORT_LEVEL=0',
      'ECORE_NO_SYSTEM_MODULES=1'
    ]
  )
endforeach

# menu render

src = files([
  'menu-render.c'
])

menu_test = executable('menu-render-test', src,
  link_with: eclair_core,
  include_directories: inc_dir_lib,
  dependencies: deps
)

foreach factor : ['1', '1.5', '2']
  test('native menu at ' + factor, menu_test,
    suite: 'display',
    env: [
      'ELM_SCALE=' + factor,
      'EINA_LOG_LEVEL=1',
      'ECORE_EVAS_COMP_NOSYNC=1',
      'EINA_LOG_ABORT=1',
      'EINA_LOG_ABORT_LEVEL=0',
      'ECORE_NO_SYSTEM_MODULES=1'
    ]
  )
  foreach theme : themes
    test('SVG menu ' + name + ' at ' + factor, menu_test,
      args: [theme],
      env: [
        'ELM_ENGINE=buffer',
	'ELM_SCALE=' + factor,
	'EINA_LOG_LEVEL=0',
        'ECORE_NO_SYSTEM_MODULES=1'
      ]
    )
  endforeach
endforeach

# menu player

src = files([
  'menu-player.c'
])

menu_player_test = executable('menu-player-test', src,
  link_with: eclair_core,
  include_directories: inc_dir_lib,
  dependencies: deps
)

foreach factor : ['1', '1.5', '2']
  test('hamburger and cascading menus stay inside all players at ' + factor,
    menu_player_test,
    args: themes,
    timeout: 40,
    env: [
      'ELM_ENGINE=buffer',
      'ELM_SCALE=' + factor,
      'EINA_LOG_LEVEL=0',
      'ECORE_NO_SYSTEM_MODULES=1'
    ]
  )
endforeach

# theme geometry

src = files([
  'theme-geometry.c'
])

geometry_test = executable('theme-geometry-test', src,
  dependencies: deps
)

test('scaled theme geometry', geometry_test,
  args: themes,
  env: [
    'EINA_LOG_LEVEL=0',
    'ECORE_NO_SYSTEM_MODULES=1'
  ]
)

# dialog

srd = files([
  'dialogs.c'
])

dialogs_test = executable('dialogs-test', src,
  link_with: eclair_core,
  include_directories: inc_dir_lib,
  dependencies: deps
)

foreach factor : ['1', '1.5', '2']
  test('Elementary dialogs at ' + factor, dialogs_test,
    suite: 'display',
    env: [
      'ELM_SCALE=' + factor,
      'EINA_LOG_LEVEL=1',
      'ECORE_EVAS_COMP_NOSYNC=1',
      'EINA_LOG_ABORT=1',
      'EINA_LOG_ABORT_LEVEL=0',
      'ECORE_NO_SYSTEM_MODULES=1'
    ]
  )

  foreach theme : themes
    name = theme.full_path().split('/')[-1]
    test('SVG playlist editor ' + name + ' at ' + factor, dialogs_test,
      args: [theme],
      env: [
        'ELM_ENGINE=buffer',
	'ELM_SCALE=' + factor,
	'EINA_LOG_LEVEL=0',
	'ECORE_NO_SYSTEM_MODULES=1'
      ]
    )
  endforeach
endforeach

# mouse input

src = files([
  'mouse-input.c'
])

mouse_test = executable('mouse-input-test', src,
  link_with: eclair_core,
  include_directories: inc_dir_lib,
  dependencies: deps
)

foreach factor : ['1', '1.5', '2']
  test('native input at ' + factor, mouse_test,
    args: [
      themes[0]
    ],
    suite: 'display',
    env: [
      'ELM_SCALE=' + factor,
      'EINA_LOG_LEVEL=1',
      'ECORE_EVAS_COMP_NOSYNC=1',
      'EINA_LOG_ABORT=1',
      'EINA_LOG_ABORT_LEVEL=0',
      'ECORE_NO_SYSTEM_MODULES=1'
    ]
  )
endforeach

# window bounds

src = files([
  'window-bounds.c'
])

bounds_test = executable('window-bounds-test', src,
  link_with: eclair_core,
  include_directories: inc_dir_lib,
  dependencies: deps
)

foreach factor : ['1', '1.5', '2']
  test('fixed player geometry at ' + factor, bounds_test,
    args: themes, suite: 'display', timeout: 100,
    env: [
      'ELM_SCALE=' + factor,
      'EINA_LOG_LEVEL=1',
      'ECORE_NO_SYSTEM_MODULES=1'
    ]
  )
endforeach

# preferences

non_player_theme = custom_target('non-player-test-theme',
  input: 'non-player.edc',
  output: 'non-player-test.edj',
  command: [ edje_cc, '@INPUT@', '@OUTPUT@' ]
)

src = files([
  'preferences.c'
])

preferences_test = executable('preferences-test', src,
  link_with: eclair_core,
  include_directories: inc_dir_lib,
  dependencies: deps
)

test('themes and XDG preferences', preferences_test,
  args: themes + [non_player_theme],
  env: [
    'ELM_SCALE=1',
    'EINA_LOG_LEVEL=0',
    'ECORE_NO_SYSTEM_MODULES=1'
  ]
)

# gant input

src = files([
  'gant-input.c'
])

gant_input_test = executable('gant-input-test', src,
  link_with: eclair_core,
  include_directories: inc_dir_lib,
  dependencies: deps
)

test('Gant mouse controls', gant_input_test,
  args: [themes[1]],
  env: [
    'ELM_SCALE=1',
    'EINA_LOG_LEVEL=0',
    'ECORE_NO_SYSTEM_MODULES=1'
  ]
)

# sliders

src = files([
  'sliders.c'
])

sliders_test = executable('sliders-test', src,
  link_with: eclair_core,
  include_directories: inc_dir_lib,
  dependencies: deps
)

test('slider mouse input', sliders_test,
  args: themes,
  env: [
    'ELM_SCALE=1',
    'EINA_LOG_LEVEL=0',
    'ECORE_NO_SYSTEM_MODULES=1'
  ]
)

# worker updates

src = files([
  'worker-updates.c'
])

worker_updates_test = executable('worker-updates-test', src,
  link_with: eclair_core,
  include_directories: inc_dir_lib,
  dependencies: deps
)

test('worker UI delivery', worker_updates_test,
  args: [
    themes[0],
    files(meson.project_source_root() / 'data/themes/default/images/body.svg')
  ],
  env: [
    'ELM_SCALE=1',
    'EINA_LOG_LEVEL=0',
    'ECORE_NO_SYSTEM_MODULES=1',
    'EINA_LOG_ABORT=1',
    'EINA_LOG_ABORT_LEVEL=0'
  ]
)

## Refresh desktop integration after a normal install (skipped for DESTDIR).

gnome = import('gnome')
gnome.post_install(
  gtk_update_icon_cache: true,
  update_desktop_database: true)

test('Folio album label follows worker cover updates', worker_updates_test,
  args: [
    themes[3],
    files(meson.project_source_root() / 'data/themes/folio/images/cover.svg')
  ],
  env: [
    'ELM_ENGINE=buffer',
    'EINA_LOG_LEVEL=0',
    'ECORE_NO_SYSTEM_MODULES=1'
  ]
)

# config

src = files([
  'config.c',
  meson.project_source_root() / 'src/lib/eclair_config.c'
])

config_test = executable('config-test', src,
  include_directories: inc_dir_lib,
  dependencies: deps
)

test('Eet settings round trip', config_test,
  env: [
    'EINA_LOG_LEVEL=0',
    'ECORE_NO_SYSTEM_MODULES=1'
  ]
)

# playlist import

src = files([
  'playlist-import.c'
])

playlist_import_test = executable('playlist-import-test', src,
  link_with: eclair_core,
  include_directories: inc_dir_lib,
  dependencies: deps
)

test('filter non-media playlist imports', playlist_import_test,
  env: [
    'EINA_LOG_LEVEL=0',
    'ECORE_NO_SYSTEM_MODULES=1'
  ]
)

#title scroll

src = files([
  'title-scroll.c'
])


title_scroll_test = executable('title-scroll-test', src,
  link_with: eclair_core,
  include_directories: inc_dir_lib,
  dependencies: deps
)

test('overflowing track titles scroll in all themes', title_scroll_test,
  args: themes,
  timeout: 75,
  env: [
    'EINA_LOG_LEVEL=0',
    'ECORE_NO_SYSTEM_MODULES=1'
  ]
)

# playlist row

src = files([
  'playlist-row.c'
])

playlist_row_test = executable('playlist-row-test', src,
  dependencies: deps)

test('playlist titles leave room for durations in all themes', playlist_row_test,
  args: themes,
  env: [
    'EINA_LOG_LEVEL=0',
    'ECORE_NO_SYSTEM_MODULES=1'
  ]
)

# playlist hover

src = files([
  'playlist-hover.c'
])

playlist_hover_test = executable('playlist-hover-test', src,
  dependencies: deps
)

test('playlist titles scroll on hover in all themes', playlist_hover_test,
  args: themes,
  timeout: 60,
  env: [
    'EINA_LOG_LEVEL=0',
    'ECORE_NO_SYSTEM_MODULES=1'
  ]
)

# external drop

## Test an actual external transfer; the application keeps using Elementary DND.

x11_test_dep = dependency('x11', required: false)
if x11_test_dep.found()

  src = files([
    'external-drop.c'
  ])

  external_drop_test = executable('external-drop-test', src,
    link_with: eclair_core,
    include_directories: inc_dir_lib,
    dependencies: deps + [x11_test_dep]
  )

  foreach theme_index : range(themes.length())
    foreach drop_kind : ['song', 'folder']
      test('external drop theme ' + theme_index.to_string() + ' ' + drop_kind,
        external_drop_test,
	args: [themes[theme_index], drop_kind],
	suite: 'display',
        env: [
	  'ELM_ENGINE=software_x11',
	  'EINA_LOG_LEVEL=0',
	  'ECORE_NO_SYSTEM_MODULES=1'
	]
      )
    endforeach
  endforeach
endif

# repeat

src = files([
  'repeat.c'
])

repeat_test = executable('repeat-test', src,
  link_with: eclair_core,
  include_directories: inc_dir_lib,
  dependencies: deps
)

test('repeat and shuffle controls and queue behavior in all themes', repeat_test,
  args: themes,
  env: [
    'EINA_LOG_LEVEL=0',
    'ECORE_NO_SYSTEM_MODULES=1'
  ]
)

# vector control

src = files([
  'vector-controls.c'
])

vector_controls_test = executable('vector-controls-test', src,
  dependencies: deps
)

test('native vector controls and alignment at all scales', vector_controls_test,
  args: themes,
  env: [
    'EINA_LOG_LEVEL=0',
    'ECORE_NO_SYSTEM_MODULES=1'
  ]
)

# nocturne

src = files([
  'nocturne.c'
])

nocturne_test = executable('nocturne-test', src,
  dependencies: deps
)

test('Nocturne playback light stops while idle', nocturne_test,
  args: [themes[2]],
  env: [
    'EINA_LOG_LEVEL=0',
    'ECORE_NO_SYSTEM_MODULES=1'
  ]
)

foreach factor : ['1', '1.5', '2']
  test('Nocturne native input at ' + factor, mouse_test,
    args: [themes[2]],
    suite: 'display',
    env: [
      'ELM_ENGINE=buffer',
      'ELM_SCALE=' + factor,
      'EINA_LOG_LEVEL=0',
      'ECORE_NO_SYSTEM_MODULES=1'
    ]
  )
endforeach

# folio

src = files([
  'folio.c',
  meson.project_source_root() / 'src/lib/eclair_turntable.c'
])

folio_test = executable('folio-test', src,
  include_directories: inc_dir_lib,
  dependencies: deps
)

foreach factor : ['1', '1.5', '2']
  test('Folio record and arm animation at ' + factor, folio_test,
    args: [
      themes[3],
      files(meson.project_source_root() / 'data/themes/folio/images/cover.svg'),
      factor
    ],
    env: [
      'EINA_LOG_LEVEL=0',
      'ECORE_NO_SYSTEM_MODULES=1'
    ]
  )
endforeach

foreach factor : ['1', '1.5', '2']
  test('Folio native input at ' + factor, mouse_test,
    args: [themes[3]],
    suite: 'display',
    env: [
      'ELM_ENGINE=buffer',
      'ELM_SCALE=' + factor,
      'EINA_LOG_LEVEL=0',
      'ECORE_NO_SYSTEM_MODULES=1'
    ]
  )
endforeach

foreach factor : ['1', '1.5', '2']
  test('Frequency native input at ' + factor, mouse_test,
    args: [themes[4]],
    suite: 'display',
    env: [
      'ELM_ENGINE=buffer',
      'ELM_SCALE=' + factor,
      'EINA_LOG_LEVEL=0',
      'ECORE_NO_SYSTEM_MODULES=1'
    ]
  )
endforeach

# frequency

src = files([
  'frequency.c'
])

frequency_test = executable('frequency-test', src,
  dependencies: deps
)

test('Frequency dial tracks position holds on pause and parks on stop',
  frequency_test,
  args: [themes[4]],
  env: [
    'EINA_LOG_LEVEL=0',
    'ECORE_NO_SYSTEM_MODULES=1'
  ]
)

foreach entry : [['Voltage', 5], ['Undertow', 6]]
  foreach factor : ['1', '1.5', '2']
    test(entry[0] + ' native input at ' + factor, mouse_test,
      args: [themes[entry[1]]], suite: 'display',
      env: [
        'ELM_ENGINE=buffer',
	'ELM_SCALE=' + factor,
	'EINA_LOG_LEVEL=0',
	'ECORE_NO_SYSTEM_MODULES=1'
      ]
    )
  endforeach
endforeach

test('Voltage signal strip stops pulsing while idle', nocturne_test,
  args: [themes[5]],
  env: [
    'EINA_LOG_LEVEL=0',
    'ECORE_NO_SYSTEM_MODULES=1'
  ]
)

# undertow

src = files([
  'undertow.c'
])

undertow_test = executable('undertow-test', src,
  dependencies: deps
)

test('Undertow sonar expands during playback and settles while idle',
  undertow_test,
  args: [themes[6]],
  env: [
    'EINA_LOG_LEVEL=0',
    'ECORE_NO_SYSTEM_MODULES=1'
  ]
)

# playback motion

src = files([
  'playback-motion.c'
])

playback_motion_test = executable('playback-motion-test', src,
  dependencies: deps
)

test('playback accents animate and settle without moving panes in every theme',
  playback_motion_test,
  args: themes,
  env: [
    'EINA_LOG_LEVEL=0',
    'ECORE_NO_SYSTEM_MODULES=1'
  ]
)

# voltage circuit

src = files([
  'voltage-circuits.c'
])

voltage_circuits_test = executable('voltage-circuits-test', src,
  dependencies: deps
)

test('Voltage lights follow existing circuit lines at all scales and stop while idle',
  voltage_circuits_test,
  args: [themes[5]],
  env: [
    'EINA_LOG_LEVEL=0',
    'ECORE_NO_SYSTEM_MODULES=1'
  ]
)
