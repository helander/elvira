package main

import (
	"embed"
	"fmt"
	"io/fs"
//	"io/ioutil"
	"log"
	"net/http"
	"net/http/cgi"
	"os"
	"path/filepath"
	"os/signal"
	"syscall"
	"context"
)

//go:embed static/* cgi-bin/*
var embeddedFiles embed.FS
/**/

var tmpCgiDir string


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
		log.Printf("CGI-skript extraherat: %s", dstPath)
	}

	return nil
}

func main() {
	// Extrahera CGI-skript
	if err := extractCGIScripts(); err != nil {
		log.Fatal("Kunde inte extrahera CGI-skript:", err)
	}
	// Skapa signalhanterare
	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt, syscall.SIGTERM)
	defer stop()

	// När programmet avslutas (normalt eller via signal)
	defer func() {
		log.Println("Rensar temporär katalog:", tmpCgiDir)
		os.RemoveAll(tmpCgiDir)
	}()

	// Statiska filer via embed
	staticFS, _ := fs.Sub(embeddedFiles, "static")
	http.Handle("/", http.FileServer(http.FS(staticFS)))

	// CGI-handler
	http.Handle("/cgi-bin/", http.StripPrefix("/cgi-bin/", http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		scriptName := filepath.Clean(r.URL.Path)
		scriptPath := filepath.Join(tmpCgiDir, scriptName)

		if _, err := os.Stat(scriptPath); os.IsNotExist(err) {
			http.NotFound(w, r)
			return
		}

		// Skapa CGI-handler med miljövariabler
		handler := &cgi.Handler{
				Path: scriptPath,
				Root: "/cgi-bin/",
				Env: []string{
					"SERVER_SOFTWARE=GoEmbeddedServer/1.0",
					"SPECIAL_VAR=HejFrånGo", // Exempel på egen variabel
					"XDG_RUNTIME_DIR="+os.Getenv("XDG_RUNTIME_DIR"), 
					"LV2_PATH="+os.Getenv("LV2_PATH"), 
				},
		}

		handler.ServeHTTP(w, r)
	})))
	port := 7000
//	log.Fatal(http.ListenAndServe(fmt.Sprintf(":%d", port), nil))

	// Starta server i en separat goroutine
	server := &http.Server{Addr: fmt.Sprintf(":%d", port)}

	go func() {
		log.Printf("Server startad på http://localhost:%d", port)
		if err := server.ListenAndServe(); err != http.ErrServerClosed {
			log.Fatal(err)
		}
	}()

	// Vänta på signal (Ctrl-C eller SIGTERM)
	<-ctx.Done()
	log.Println("Avslutar...")

	// Stäng av servern snyggt
	server.Shutdown(context.Background())

}


