// =====================================================================================================
// File:           controls.go
// Project:        elvira
// Author:         Lars-Erik Helander <lehswel@gmail.com>
// License:        MIT
// Description:    Web server service handler for fetching controls data for a node
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

// Output data types
type Point struct {
        Label       string     `json:"label"`
        Value       float64    `json:"value"`
}

type Endpoint struct {
        Element       string   `json:"element"`
        Index         *int     `json:"index,omitempty"`
        Uri           string   `json:"uri,omitempty"`
}

type View struct {
        Element       string   `json:"element"`
        Min           *float64 `json:"min,omitempty"`
        Max           *float64 `json:"max,omitempty"`
        Integer       bool     `json:"integer,omitempty"`
        Points        []Point  `json:"points,omitempty"`
}

type Pair struct {
        Name       string   `json:"name"`
	View       View     `json:"view"`
	Endpoint   Endpoint `json:"endpoint"`
        Prio       float64  `json:"prio,omitempty"`
}


// Input data types
type Prop struct {
        Uri           string   `json:"uri,omitempty"`
        Range         string   `json:"range,omitempty"`
        Name          string   `json:"name,omitempty"`
        Symbol        string   `json:"symbol,omitempty"`
        Prio          float64  `json:"prio,omitempty"`
        Scale         []Point  `json:"scale,omitempty"`
        Index         int      `json:"index,omitempty"`
        Midicc        int      `json:"midicc,omitempty"`
        Input         bool     `json:"input,omitempty"`
        Control       bool     `json:"control,omitempty"`
        Audio         bool     `json:"audio,omitempty"`
        Enum          bool     `json:"enum,omitempty"`
        Toggle        bool     `json:"toggle,omitempty"`
        Min           float64  `json:"min,omitempty"`
        Max           float64  `json:"max,omitempty"`
        Default       float64  `json:"default,omitempty"`
}

// =====================================================================================================
// Local state
// =====================================================================================================

/*
var examples = []Pair{
  {
    Endpoint: Endpoint{Element: "lv2parameter-endpoint", Uri: "urn:ardour:a-fluidsynth:sf2file"},
    View: View{Element: "filepath-view"},
    Name: "Dynamic 100",
    Prio: 10,
  },
  {
    Endpoint: Endpoint{Element: "midicc-endpoint", Index: 7},
    View: View{Element: "slider-view", Min: 0, Max: 127, Integer: false},
    Name: "Dynamic 2",
    Prio: 500,
  },
  {
    Endpoint: Endpoint{Element: "lv2control-endpoint", Index: 4},
    View: View{Element: "select-view", Points : []Point{Point{Label: "Cow", Value: 32},Point{Label: "Cat", Value: 67},Point{Label: "Hound", Value: 12}}},
    Name: "Dynamic 3",
    Prio: 1,
  },
}
*/

// =====================================================================================================
// Local functions
// =====================================================================================================

func getNode(nodeID int) (interface{},interface{},interface{}, error) {
	cmd := exec.Command("pw-dump")
	out, err := cmd.Output()
	if err != nil {
		return nil,nil,nil, fmt.Errorf("failed to run pw-dump: %v", err)
	}

	var all []map[string]interface{}
	if err := json.Unmarshal(out, &all); err != nil {
		return nil,nil,nil, fmt.Errorf("failed to parse pw-dump output: %v", err)
	}

	//var result []map[string]interface{}
	for _, obj := range all {
		if obj["type"] != "PipeWire:Interface:Node" {
			continue
		}

		if nodeID >= 0 {
			idFloat, ok := obj["id"].(float64)
			if !ok || int(idFloat) != nodeID {
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
                return props["elvira.host.info.ports"].(interface{}), props["elvira.host.midi.params"].(interface{}), props["elvira.host.info.params"].(interface{}), nil


	}

	return nil, nil, nil, fmt.Errorf("No node data found") 
}

// =====================================================================================================
// pairsHandler
// =====================================================================================================
func pairsHandler(w http.ResponseWriter, r *http.Request) {
	parts := strings.Split(strings.TrimPrefix(r.URL.Path, "/pairs/"), "/")
	if len(parts) == 0 || parts[0] == "" {
		http.Error(w, "missing node ID", 400)
		return
	}

	nodeID, err := strconv.Atoi(parts[0])
	if err != nil {
		http.Error(w, "invalid node ID", 400)
		return
	}


	ports, midi, params, err := getNode(nodeID)
	if err != nil {
		http.Error(w, err.Error(), 500)
		return
	}
        //log.Printf("Ports %s",ports)
        //log.Printf("Midi %s",midi)
        log.Printf("Params %s",params)

        var dports []Prop 
	if err := json.Unmarshal([]byte(ports.(string)), &dports); err != nil {
		http.Error(w, err.Error(), 500)
		return
        }
        log.Printf("Ports %v",dports)

        var dmidis []Prop 
	if err := json.Unmarshal([]byte(midi.(string)), &dmidis); err != nil {
		http.Error(w, err.Error(), 500)
		return
        }
        log.Printf("Midis %v",dmidis)

        var dparams []Prop 
	if err := json.Unmarshal([]byte(params.(string)), &dparams); err != nil {
		http.Error(w, err.Error(), 500)
		return
        }
        log.Printf("Params %v",dparams)


        pairs := make([]Pair, 0)

        for _, port := range dports {
	    log.Printf("Port: %v", port)
            if port.Input && port.Control {
                endpoint := Endpoint{Element: "lv2control-endpoint", Index: &port.Index}
                view := View{}
                if port.Enum || port.Toggle {
                  view.Element ="select-view"
                  view.Points = port.Scale
                } else {
                  view.Element ="slider-view"
                  view.Min = &port.Min
                  view.Max = &port.Max
                  view.Integer = true
                }
                pair := Pair{Endpoint: endpoint, View: view, Name: port.Name }
                pairs = append(pairs, pair)
           }
        }

        for _, midi := range dmidis {
            endpoint := Endpoint{Element: "midicc-endpoint", Index: &midi.Midicc}
            view := View{}
            if (midi.Enum || midi.Toggle) {
              view.Element ="select-view"
              view.Points = midi.Scale
            } else {
              view.Element ="slider-view"
              view.Min = &midi.Min
              view.Max = &midi.Max
              view.Integer = true
            }
            pair := Pair{Endpoint: endpoint, View: view, Name: midi.Name }
            pairs = append(pairs, pair)
        }

        for _, param := range dparams {
	    log.Printf("Param: %v", param)
            endpoint := Endpoint{Element: "lv2parameter-endpoint", Uri: param.Uri}
            view := View{}
            if param.Range == "http://lv2plug.in/ns/ext/atom#Path" {
              view.Element ="filepath-view"
            } else {
              view.Element ="select-view"
              view.Points = []Point{Point{Label: "TBD", Value: 0},Point{Label: "TBD", Value: 100}}
            }
            pair := Pair{Endpoint: endpoint, View: view, Name: param.Name }
            pairs = append(pairs, pair)
        }

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(pairs)
}


// =====================================================================================================
// Init
// =====================================================================================================
func init() {
	http.HandleFunc("/pairs/", pairsHandler) 

}
