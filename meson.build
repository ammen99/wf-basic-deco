project(
	'wf-decorator',
    'c',
	'cpp',
	version: '0.0',
	license: 'MIT',
	meson_version: '>=0.43.0',
	default_options: [
		'cpp_std=c++11',
        'c_std=c99',
		'warning_level=2',
		'werror=false',
	],
)

cairo = dependency('cairo')
wayland_client = dependency('wayland-client')
wayland_protos = dependency('wayland-protocols')
wayland_cursor = dependency('wayland-cursor')

subdir('proto')
subdir('src')
