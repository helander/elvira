// =====================================================================================================
// File:           elwira.go
// Project:        elvira
// Author:         Lars-Erik Helander <lehswel@gmail.com>
// License:        MIT
// Description:    Web server with CGI support and embedded static content
// =====================================================================================================

package main

import (
	"context"
	"embed"
	"encoding/json"
	"fmt"
	"io/fs"
	"log"
	"net/http"
	"net/http/cgi"
	"os"
	"os/exec"
	"os/signal"
	"path/filepath"
	"strconv"
	"strings"
	"syscall"
	"time"

        "elwira/lilv"
)

// =====================================================================================================
// Types & constants
// =====================================================================================================

// =====================================================================================================
// Local state
// =====================================================================================================
//go:embed static/* cgi-bin/*
var embeddedFiles embed.FS
/**/

var tmpCgiDir string

// =====================================================================================================
// Local functions
// =====================================================================================================
func extractCGIScripts() error {
	entries, err := fs.ReadDir(embeddedFiles, "cgi-bin")
	if err != nil {
		return err
	}

	tmpCgiDir, err = os.MkdirTemp("", "cgi-bin-*")
	if err != nil {
		return err
	}

	for _, entry := range entries {
		name := entry.Name()
		data, err := embeddedFiles.ReadFile("cgi-bin/" + name)
		if err != nil {
			return err
		}
		dstPath := filepath.Join(tmpCgiDir, name)
		err = os.WriteFile(dstPath, data, 0755)
		if err != nil {
			return err
		}
		log.Printf("CGI-script extracted: %s", dstPath)
	}

	return nil
}

func pluginsHandler(w http.ResponseWriter, r *http.Request) {
    if r.Method != http.MethodGet {
        http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
        return
    }
    lilv.Plugins(w)
}

func presetsHandler(w http.ResponseWriter, r *http.Request) {
    if r.Method != http.MethodGet {
        http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
        return
    }

    uriParam := r.URL.Query().Get("uri")
    if uriParam == "" {
        http.Error(w, "Missing 'uri' parameter", http.StatusBadRequest)
        return
    }

    lilv.Presets(w,uriParam)
}

func handlePipewireVolume(w http.ResponseWriter, r *http.Request) {
	parts := strings.Split(strings.TrimPrefix(r.URL.Path, "/pw-volume/"), "/")
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

func handlePipewireMetadata(w http.ResponseWriter, r *http.Request) {
	parts := strings.Split(strings.TrimPrefix(r.URL.Path, "/pw-metadata/"), "/")
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

func handlePipewireNodeProps(w http.ResponseWriter, r *http.Request) {
	parts := strings.Split(strings.TrimPrefix(r.URL.Path, "/pw-node-props/"), "/")
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
	return strings.HasPrefix(s, "{") || strings.HasPrefix(s, "[")
}



// =====================================================================================================
// Main
// =====================================================================================================
func main() {
	if err := extractCGIScripts(); err != nil {
		log.Fatal("Error extracting CGI-script:", err)
	}
	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt, syscall.SIGTERM)
	defer stop()

	defer func() {
		log.Println("Clean temporary storage:", tmpCgiDir)
		os.RemoveAll(tmpCgiDir)
	}()


	// Embedded static files
	staticFS, _ := fs.Sub(embeddedFiles, "static")
	http.Handle("/", http.FileServer(http.FS(staticFS)))

        // lilv services
        http.HandleFunc("/plugins", pluginsHandler)
        http.HandleFunc("/presets", presetsHandler)
	http.HandleFunc("/pw-node-props/", handlePipewireNodeProps) 
	http.HandleFunc("/pw-metadata/", handlePipewireMetadata) 
	http.HandleFunc("/pw-volume/", handlePipewireVolume) 

	// CGI-handler
	http.Handle("/cgi-bin/", http.StripPrefix("/cgi-bin/", http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		scriptName := filepath.Clean(r.URL.Path)
		scriptPath := filepath.Join(tmpCgiDir, scriptName)

		if _, err := os.Stat(scriptPath); os.IsNotExist(err) {
			http.NotFound(w, r)
			return
		}

		handler := &cgi.Handler{
				Path: scriptPath,
				Root: "/cgi-bin/",
				Env: []string{
					"PWD="+os.Getenv("PWD"), 
					"HOME="+os.Getenv("HOME"), 
					"XDG_RUNTIME_DIR="+os.Getenv("XDG_RUNTIME_DIR"), 
					"LV2_PATH="+os.Getenv("LV2_PATH"), 
					"DISPLAY="+os.Getenv("DISPLAY"), 
				},
		}

		handler.ServeHTTP(w, r)
	})))
	port := 7000


	server := &http.Server{
		Addr:         fmt.Sprintf(":%d", port),
		ReadTimeout:  5 * time.Second,
		WriteTimeout: 10 * time.Second,
		IdleTimeout:  120 * time.Second,
	}


	go func() {
		log.Printf("LV2_PATH=%s", os.Getenv("LV2_PATH"))
		log.Printf("Server started at http://localhost:%d", port)
		if err := server.ListenAndServe(); err != http.ErrServerClosed {
			log.Fatal(err)
		}
	}()

	// Wait for signal (Ctrl-C or SIGTERM)
	<-ctx.Done()
	log.Println("Terminating...")

	// Clean server shutdown
	server.Shutdown(context.Background())

}

