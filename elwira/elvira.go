package main

import (
	"net/http"
	"os"
	"os/exec"
	"syscall"
)

func init() {
	http.HandleFunc("/elvira", elviraHandler)

}

// Add signal handler to force removal of defunct child processes
func elviraHandler(w http.ResponseWriter, r *http.Request) {
	//if err := r.ParseForm(); err != nil {
	//	http.Error(w, "Fel vid formulärtolkning", 400)
	//	return
	//}


        name := r.URL.Query()["name"][0]
        uri := r.URL.Query()["uri"][0]
        showui := r.URL.Query()["showui"][0]
        step := r.URL.Query()["step"][0]

        if (showui == "true") {
          showui = "--showui"
        } else {
          showui = ""
        }

	// Skapa temporär fil och ta bort den direkt (unlink)
	tmpfile, err := os.CreateTemp("", "log")
	if err != nil {
		http.Error(w, "Kunde inte skapa tempfil", 500)
		return
	}
	defer tmpfile.Close()
	os.Remove(tmpfile.Name()) // unlink direkt

	cmd := exec.Command("elvira", name, uri, "--step", step,showui)
	cmd.Stdout = tmpfile
	cmd.Stderr = tmpfile
	cmd.ExtraFiles = []*os.File{tmpfile}

    	cmd.SysProcAttr = &syscall.SysProcAttr{
        	Setsid: true,
    	}

	if err := cmd.Start(); err != nil {
		http.Error(w, "Kunde inte starta processen", 500)
		return
	}

}

