// =====================================================================================================
// File:           pw.go
// Project:        elvira
// Author:         Lars-Erik Helander <lehswel@gmail.com>
// License:        MIT
// Description:    Web server service handlers for pipewire related services
// =====================================================================================================

package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"os/exec"
	"strconv"
	"strings"
)

// =====================================================================================================
// Types & constants
// =====================================================================================================
type PWObject struct {
	ID   int `json:"id"`
	Type string `json:"type"`
	Info struct {
		Props map[string]interface{} `json:"props"`
	} `json:"info"`
}

type NodeInfo struct {
	ID     int     `json:"id"`
	Group   string  `json:"group,omitempty"`
	Step   float64  `json:"step"`
	Name   string  `json:"name,omitempty"`
	Pid    float64     `json:"pid"`
	Gain   float64 `json:"gain"`
	Plugin string  `json:"plugin,omitempty"`
	Preset string  `json:"preset,omitempty"`
}

// =====================================================================================================
// Local state
// =====================================================================================================

// =====================================================================================================
// Local functions
// =====================================================================================================
func stringVal(v interface{}) string {
	if v == nil {
		return ""
	}
	if s, ok := v.(string); ok {
		return s
	}
	return ""
}


func getFilteredNodes(prefixes []string, onlyID int) ([]map[string]interface{}, error) {
	cmd := exec.Command("pw-dump")
	out, err := cmd.Output()
	if err != nil {
		return nil, fmt.Errorf("failed to run pw-dump: %v", err)
	}

	var all []map[string]interface{}
	if err := json.Unmarshal(out, &all); err != nil {
		return nil, fmt.Errorf("failed to parse pw-dump output: %v", err)
	}

	var result []map[string]interface{}
	for _, obj := range all {
		if obj["type"] != "PipeWire:Interface:Node" {
			continue
		}

		if onlyID >= 0 {
			idFloat, ok := obj["id"].(float64)
			if !ok || int(idFloat) != onlyID {
				continue
			}
		}

		info, ok := obj["info"].(map[string]interface{})
		if !ok {
			continue
		}

		props, ok := info["props"].(map[string]interface{})
		if !ok || props == nil {
			continue
		}

		if !propsHaveAnyPrefix(props, prefixes) {
			continue
		}

		outProps := make(map[string]interface{})
		for k, v := range props {
			if hasAnyPrefix(k, prefixes) {
				if str, ok := v.(string); ok && looksLikeJSON(str) {
					var parsed interface{}
					if err := json.Unmarshal([]byte(str), &parsed); err == nil {
						outProps[k] = parsed
						continue
					} else {
						fmt.Printf("\nPARSE Error (%v)",err)
					}
				}
				outProps[k] = v
			}
		}

		result = append(result, map[string]interface{}{
			"id":         obj["id"],
			"properties": outProps,
		})
	}

	return result, nil
}

func hasAnyPrefix(key string, prefixes []string) bool {
	for _, prefix := range prefixes {
		if strings.HasPrefix(key, prefix) {
			return true
		}
	}
	return false
}

func propsHaveAnyPrefix(props map[string]interface{}, prefixes []string) bool {
	for k := range props {
		if hasAnyPrefix(k, prefixes) {
			return true
		}
	}
	return false
}

func looksLikeJSON(s string) bool {
	s = strings.TrimSpace(s)
	result := strings.HasPrefix(s, "{") || strings.HasPrefix(s, "[")
        return result
}

// =====================================================================================================
// NodesHandler
// =====================================================================================================
func NodesHandler(w http.ResponseWriter, r *http.Request) {
	cmd := exec.Command("pw-dump")
	output, err := cmd.Output()
	if err != nil {
		http.Error(w, "Failed to run pw-dump", http.StatusInternalServerError)
		log.Println("pw-dump error:", err)
		return
	}

	var all []PWObject
	if err := json.Unmarshal(output, &all); err != nil {
		http.Error(w, "Failed to parse pw-dump JSON", http.StatusInternalServerError)
		log.Println("unmarshal error:", err)
		return
	}

	var result []NodeInfo
	for _, obj := range all {
		if obj.Type != "PipeWire:Interface:Node" {
			continue
		}
		role, ok := obj.Info.Props["elvira.role"]
		if !ok || role != "instance" {
			continue
		}
		props := obj.Info.Props
		node := NodeInfo{
			ID:     obj.ID,
			Group:   stringVal(props["elvira.group"]),
			Step:   props["elvira.step"].(float64),
			Name:   stringVal(props["node.name"]),
			Pid:    props["elvira.pid"].(float64),
			Gain:   props["elvira.gain"].(float64),
			Plugin: stringVal(props["elvira.plugin"]),
			Preset: stringVal(props["elvira.preset"]),
		}
		result = append(result, node)
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(result)
}



// =====================================================================================================
// VolumeHandler
// =====================================================================================================
func VolumeHandler(w http.ResponseWriter, r *http.Request) {
	parts := strings.Split(strings.TrimPrefix(r.URL.Path, "/volume/"), "/")
	if len(parts) == 0 || parts[0] == "" {
		http.Error(w, "missing node ID", 400)
		return
	}

	nodeID, err := strconv.Atoi(parts[0])
	if err != nil {
		http.Error(w, "invalid node ID", 400)
		return
	}

	gain := r.URL.Query()["gain"][0]

	cmd := exec.Command("pw-cli","--","set-param",fmt.Sprintf("%d",nodeID),"Props",fmt.Sprintf("{volume=%s}", gain))
	_, err = cmd.Output()
	if err != nil {
		http.Error(w, "failed to run pw-volume", 500)
		return
	}
}

// =====================================================================================================
// NodeHandler -- move this to elvira.go (delete instance)
// =====================================================================================================
func NodeHandler(w http.ResponseWriter, r *http.Request) {

	parts := strings.Split(strings.TrimPrefix(r.URL.Path, "/nodes/"), "/")
	if len(parts) == 0 || parts[0] == "" {
		http.Error(w, "missing node ID", 400)
		return
	}

	//nodeID, err := strconv.Atoi(parts[0])
	//if err != nil {
	//	http.Error(w, "invalid node ID", 400)
	//	return
	//}

    if r.Method != http.MethodDelete {
        http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
        return
    }

	pid := r.URL.Query()["pid"][0]

        cmd := exec.Command("kill","-9",pid)
        //cmd := exec.Command("pw-cli","destroy",fmt.Sprintf("%d",nodeID))

	_, err := cmd.Output()
	if err != nil {
		http.Error(w, "failed to run node destruction", 500)
		return
	}

}

// =====================================================================================================
// MetadataHandler
// =====================================================================================================
func MetadataHandler(w http.ResponseWriter, r *http.Request) {
	parts := strings.Split(strings.TrimPrefix(r.URL.Path, "/metadata/"), "/")
	if len(parts) == 0 || parts[0] == "" {
		http.Error(w, "missing node ID", 400)
		return
	}

	nodeID, err := strconv.Atoi(parts[0])
	if err != nil {
		http.Error(w, "invalid node ID", 400)
		return
	}

	key := r.URL.Query()["key"][0]
	value := r.URL.Query()["value"][0]

	cmd := exec.Command("pw-metadata","--",fmt.Sprintf("%d",nodeID), key, value)
	_, err = cmd.Output()
	if err != nil {
		http.Error(w, "failed to run pw-metadata", 500)
		return
	}
}

// =====================================================================================================
// NodePropsHandler
// =====================================================================================================
func NodePropsHandler(w http.ResponseWriter, r *http.Request) {
	parts := strings.Split(strings.TrimPrefix(r.URL.Path, "/node-props/"), "/")
	if len(parts) == 0 || parts[0] == "" {
		http.Error(w, "missing node ID", 400)
		return
	}

	nodeID, err := strconv.Atoi(parts[0])
	if err != nil {
		http.Error(w, "invalid node ID", 400)
		return
	}

	prefixes := r.URL.Query()["prefix"]
	if len(prefixes) == 0 {
		prefixes = []string{"elvira."}
	}

	nodes, err := getFilteredNodes(prefixes, nodeID)
	if err != nil {
		http.Error(w, err.Error(), 500)
		return
	}

	if len(nodes) == 0 {
		http.NotFound(w, r)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(nodes[0])
}


// =====================================================================================================
// Init
// =====================================================================================================
func init() {
        http.HandleFunc("/nodes/", NodeHandler)
        http.HandleFunc("/nodes", NodesHandler)
	http.HandleFunc("/node-props/", NodePropsHandler) 
	http.HandleFunc("/metadata/", MetadataHandler) 
	http.HandleFunc("/volume/", VolumeHandler) 
}

