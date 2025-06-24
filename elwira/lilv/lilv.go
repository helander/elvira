package lilv

/*
#cgo pkg-config: lilv-0
#include <lilv/lilv.h>
#include <stdlib.h>

*/
import "C"
import "fmt"
import "net/http"
import "unsafe"

func Plugins(w http.ResponseWriter) {
    world := C.lilv_world_new()
    C.lilv_world_load_all(world)

    plugins := C.lilv_world_get_all_plugins(world)

    fmt.Fprintf(w,"[")
    first := true

    for i := C.lilv_plugins_begin(plugins); !C.lilv_plugins_is_end(plugins, i); i = C.lilv_plugins_next(plugins, i) {
        plugin := C.lilv_plugins_get(plugins, i)
        uri := C.lilv_plugin_get_uri(plugin)
        str := C.lilv_node_as_uri(uri)

        if !first {
            fmt.Fprintf(w, ",")
        }
        first = false
        fmt.Fprintf(w, "\"%s\"", C.GoString(str))
    }

    fmt.Fprintf(w, "]")

    C.lilv_world_free(world)
}

func Presets(w http.ResponseWriter, pluginURI string) {
	cURI := C.CString(pluginURI)
	defer C.free(unsafe.Pointer(cURI))

	world := C.lilv_world_new()
	defer C.lilv_world_free(world)

	C.lilv_world_load_all(world)

	pluginUriNode := C.lilv_new_uri(world, cURI)
	defer C.lilv_node_free(pluginUriNode)

	plugins := C.lilv_world_get_all_plugins(world)

	var plugin *C.LilvPlugin
	for iter := C.lilv_plugins_begin(plugins); !C.lilv_plugins_is_end(plugins, iter); iter = C.lilv_plugins_next(plugins, iter) {
		p := C.lilv_plugins_get(plugins, iter)
		pURI := C.lilv_plugin_get_uri(p)

		if C.GoString(C.lilv_node_as_uri(pURI)) == pluginURI {
			plugin = p
			break
		}
	}

	if plugin == nil {
		fmt.Fprintf(w, "[]")
		return
	}

	presetClass := C.lilv_new_uri(world, C.CString("http://lv2plug.in/ns/ext/presets#Preset"))
	defer C.lilv_node_free(presetClass)

	presets := C.lilv_plugin_get_related(plugin, presetClass)
	if presets == nil || C.lilv_nodes_size(presets) == 0 {
		fmt.Fprintf(w, "[]")
		if presets != nil {
			C.lilv_nodes_free(presets)
		}
		return
	}
	defer C.lilv_nodes_free(presets)

	labelPred := C.lilv_new_uri(world, C.CString("http://www.w3.org/2000/01/rdf-schema#label"))
	defer C.lilv_node_free(labelPred)

	fmt.Fprintf(w, "[")
	first := true
	for iter := C.lilv_nodes_begin(presets); !C.lilv_nodes_is_end(presets, iter); iter = C.lilv_nodes_next(presets, iter) {
		preset := C.lilv_nodes_get(presets, iter)
		C.lilv_world_load_resource(world, preset)

		if !first {
			fmt.Fprintf(w, ",")
		}
		first = false

		fmt.Fprintf(w, "{\"uri\":\"%s\"", C.GoString(C.lilv_node_as_uri(preset)))

		labels := C.lilv_world_find_nodes(world, preset, labelPred, nil)
		if labels != nil {
			if C.lilv_nodes_size(labels) > 0 {
				label := C.lilv_nodes_get_first(labels)
				fmt.Fprintf(w, ",\"label\":\"%s\"", C.GoString(C.lilv_node_as_string(label)))
			}
			C.lilv_nodes_free(labels)
		}

		fmt.Fprintf(w, "}")
	}
	fmt.Fprintf(w, "]")
}
