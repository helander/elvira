package main

import (
	"fmt"
	"net/http"
	"os"
	"os/exec"
	"syscall"
)

func init() {
	http.HandleFunc("/extra", indexHandler)
	http.HandleFunc("/elvira", elviraHandler)

}

func indexHandler(w http.ResponseWriter, r *http.Request) {
	fmt.Fprintf(w, `
	<html><body>
	<h2>Starta bakgrundsprogram</h2>
	<form action="/start" method="POST">
		Kommando: <input name="cmd" value="bash -c 'for i in {1..40}; do echo rad $i; sleep 1; done'">
		<input type="submit" value="Starta">
	</form>
	</body></html>`)
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

        if (showui == "true") {
          showui = "--showui"
        } else {
          showui = ""
        }
	//cmdline := r.FormValue("cmd")
	//if cmdline == "" {
	//	http.Error(w, "Kommando saknas", 400)
	//	return
	//}

	// Skapa temporär fil och ta bort den direkt (unlink)
	tmpfile, err := os.CreateTemp("", "log")
	if err != nil {
		http.Error(w, "Kunde inte skapa tempfil", 500)
		return
	}
	defer tmpfile.Close()
	os.Remove(tmpfile.Name()) // unlink direkt

	cmd := exec.Command("elvira", name, uri, showui)
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

	//pid := cmd.Process.Pid
	//http.Redirect(w, r, fmt.Sprintf("/logs/%d", pid), http.StatusSeeOther)
}

