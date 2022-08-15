# wf-ctrl
A wayfire plugin and program to control wayfire surfaces and desktop.

## Build

meson build --prefix=/usr

ninja -C build

sudo ninja -C build install

## Runtime

Enable Control Protocol plugin

Run `wf-ctrl` and pass the view ID with -i. You can find the view ID with wf-info.

## Examples

```
$ wf-ctrl -i xxxxxxxxx --move 954,384
$ wf-ctrl -i xxxxxxxxx --resize 1024,768
$ wf-ctrl -i xxxxxxxxx --unminimize
$ wf-ctrl -i xxxxxxxxx --maximize
$ wf-ctrl -i xxxxxxxxx --focus
# Get ID from wf-info
$ wf-ctrl -i $(wf-info|grep "View ID"|awk '{print $3}') --minimize
# Specify multiple view ids
wf-ctrl -i xxxxxxxxx -i xxxxxxxxx -i xxxxxxxxx --switch-ws 1,0
```