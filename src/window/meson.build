libwfwindowutil = static_library('wfwindow',
    ['window.cpp', 'shm-surface.cpp', 'input.cpp', 'decoration-protocol.cpp'],
    dependencies: [cairo, wf_protos])


wfwinheaders = include_directories('.')
wf_window_util = declare_dependency(link_with: libwfwindowutil, include_directories: wfwinheaders)
