package node
    // #cgo LDFLAGS: -g -lpipewire-0.3 -llilv-0 -ldl -lsord-0 -lserd-0 -lsratom-0 -lsuil-0  -lgtk-3
    // #cgo CFLAGS: -g -I/usr/include/pipewire-0.3 -I/usr/include/spa-0.2 -D_REENTRANT
    // -I/usr/include/lilv-0 -I/usr/include/suil-0 -I/usr/local/include -I/usr/include/serd-0
    // -I/usr/include/sord-0 -I/usr/include/sratom-0 -I/usr/include/gtk-3.0 -I/usr/include/pango-1.0
    // -I/usr/include/glib-2.0  -I/usr/lib/aarch64-linux-gnu/glib-2.0/include
    // -I/usr/include/harfbuzz    -I/usr/include/cairo    -I/usr/include/gdk-pixbuf-2.0
    // -I/usr/include/atk-1.0 -pthread
    // #include "program.h"
    // #include "node.h"
    import "C" import "unsafe"

    func
    Init(){C.program_init()}

func GtkMain(){C.program_gtk_main()}

func Plugin(pluginUri string) unsafe
    .Pointer{return C.node_plugin(C.CString(pluginUri)) return nil}

func Preset(plugin unsafe.Pointer, presetUri string) unsafe.Pointer{
    return C.node_preset(plugin, C.CString(presetUri)) return nil}

func Instance(plugin unsafe.Pointer, preset unsafe.Pointer, instanceName string, sampleRate int,
              period int) {
   go func(){C.node_thread_loop(plugin, preset, C.CString(instanceName), C.int(sampleRate),
                                C.int(period))}()
}
