project(
	'wf-decorator',
    'c',
	'cpp',
	version: '0.0',
	license: 'MIT',
	meson_version: '>=0.51.0',
	default_options: [
		'cpp_std=c++17',
        'c_std=c99',
		'warning_level=2',
		'werror=false',
	],
)

gtk3 = dependency('gtk+-3.0')
wayfire = dependency('wayfire')
wlroots = dependency('wlroots')
wayland_client = dependency('wayland-client')
wayland_server = dependency('wayland-server')

project_args = ['-DWLR_USE_UNSTABLE']
add_project_arguments(project_args, language: ['cpp', 'c'])

subdir('proto')
subdir('wf-gtk3-decor')
subdir('wf-plugin')
