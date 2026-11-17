
inc_dir_lib = include_directories('.')

src = files([
  'eclair.c',
  'eclair_scale.c',
  'eclair_args.c',
  'eclair_callbacks.c',
  'eclair_config.c',
  'eclair_cover.c',
  'eclair_cover_network.c',
  'eclair_dialogs.c',
  'eclair_media_file.c',
  'eclair_menu.c',
  'eclair_ui.c',
  'eclair_meta_tag.c',
  'eclair_playlist.c',
  'eclair_playlist_container.c',
  'eclair_subtitles.c',
  'eclair_utils.c',
  'eclair_video.c',
  'eclair_window.c',
  'eclair_turntable.c',
])

eclair_core = static_library('eclair-core', src,
  include_directories: inc_dir,
  dependencies: deps
)
