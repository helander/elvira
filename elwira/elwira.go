// =====================================================================================================
// File:           elwira.go
// Project:        elvira
// Author:         Lars-Erik Helander <lehswel@gmail.com>
// License:        MIT
// Description:    Web server with CGI support and embedded static content
// =====================================================================================================

package main

import (
	"embed"
	"fmt"
	"io/fs"
	"log"
	"net/http"
	"net/http/cgi"
	"os"
	"path/filepath"
	"os/signal"
	"syscall"
	"context"

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

// =====================================================================================================
// Main
// =====================================================================================================
func main() {
        //lilv.Y("http://drobilla.net/plugins/mda/DX10")
        //lilv.X()
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

	server := &http.Server{Addr: fmt.Sprintf(":%d", port)}

	go func() {
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

