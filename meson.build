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

gtk3 = dependency('gtk+-3.0')
wayland_client = dependency('wayland-client')

subdir('proto')

deco = executable('wf-decorator',
    ['main.cpp', 'protocol.cpp'],
    dependencies: [gtk3, wayland_client, wf_protos])
