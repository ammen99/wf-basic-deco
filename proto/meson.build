wayland_scanner = find_program('wayland-scanner')
wl_protocol_dir = wayland_protos.get_pkgconfig_variable('pkgdatadir')

wayland_scanner_code = generator(
	wayland_scanner,
	output: '@BASENAME@-protocol.c',
	arguments: ['private-code', '@INPUT@', '@OUTPUT@'],
)

wayland_scanner_client = generator(
	wayland_scanner,
	output: '@BASENAME@-client-protocol.h',
	arguments: ['client-header', '@INPUT@', '@OUTPUT@'],
)

client_protocols = [
	[wl_protocol_dir, 'stable/xdg-shell/xdg-shell.xml'],
    'wf-decorator.xml'
]

wf_protos_src = []
wf_protos_headers = []

foreach p : client_protocols
	xml = join_paths(p)
	wf_protos_headers += wayland_scanner_client.process(xml)
    wf_protos_src += wayland_scanner_code.process(xml)
endforeach

lib_wf_protos = static_library('wf_protos', wf_protos_src + wf_protos_headers,
	dependencies: [wayland_client])

wf_protos = declare_dependency(link_with: lib_wf_protos, sources: wf_protos_headers)
