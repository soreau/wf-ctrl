sources = ['main.cpp', 'plugin/wayfire-control.cpp']

wf_ctrl = shared_module('wf-ctrl', sources,
    dependencies: [wayfire, wf_server_protos],
    install: true, install_dir: join_paths(get_option('libdir'), 'wayfire'))
    
subdir('client')
