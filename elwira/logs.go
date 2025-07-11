package main

import (
	"html/template"
	"io"
	"net/http"
	"os"
	"path"
	"strconv"
	"strings"
)

func init() {
	http.HandleFunc("/logs/", logHandler)

}


// logHandler handles both HTML and /text variant
func logHandler(w http.ResponseWriter, r *http.Request) {
	parts := strings.Split(strings.TrimPrefix(r.URL.Path, "/logs/"), "/")
	if len(parts) == 0 || parts[0] == "" {
		http.Error(w, "Bad request", 400)
		return
	}
	pidStr := parts[0]
	pid, err := strconv.Atoi(pidStr)
	if err != nil {
		http.Error(w, "Illegal PID", 400)
		return
	}
	if len(parts) == 2 && parts[1] == "text" {
		serveLogText(w, pid)
	} else {
		serveLogPage(w, pid)
	}
}

func serveLogText(w http.ResponseWriter, pid int) {
	procPath := path.Join("/proc", strconv.Itoa(pid), "fd/1")
	f, err := os.Open(procPath)
	if err != nil {
		http.Error(w, "Log not available", 404)
		return
	}
	defer f.Close()
	io.Copy(w, f)
}

func serveLogPage(w http.ResponseWriter, pid int) {
	tmpl := `
	<html><body>
	<h2>Log for PID {{.PID}}</h2>
	<pre id="log" style="height: 400px; overflow-y: auto; background: #f0f0f0; padding: 10px;"></pre>
	<br/>
	<button id="toggle">Stop updating</button>
	<script>
	let interval = null;
	const logElem = document.getElementById("log");
	const toggleBtn = document.getElementById("toggle");

	function updateLog() {
		fetch(location.pathname + "/text")
			.then(res => res.text())
			.then(text => {
				logElem.textContent = text;
				logElem.scrollTop = logElem.scrollHeight;
			});
	}

	function startUpdating() {
		if (!interval) {
			interval = setInterval(updateLog, 3000);
			toggleBtn.textContent = "Stop updating";
		}
	}

	function stopUpdating() {
		if (interval) {
			clearInterval(interval);
			interval = null;
			toggleBtn.textContent = "Start updating";
		}
	}

	toggleBtn.addEventListener("click", () => {
		if (interval) {
			stopUpdating();
		} else {
			startUpdating();
			updateLog();
		}
	});

	startUpdating();
	updateLog();
	</script>
	<br/><br/>
	<a href="/">← Back</a>
	</body></html>`

	t := template.Must(template.New("logpage").Parse(tmpl))
	t.Execute(w, struct{ PID int }{PID: pid})
}
