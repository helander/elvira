package main

import (
    "encoding/json"
    "net/http"
    "os"
    "path/filepath"
    "strconv"
)

func init() {
    http.HandleFunc("/listdir", listDirectory)
}

func listDirectory(w http.ResponseWriter, r *http.Request) {
    path := r.URL.Query().Get("path")
    if path == "" {
        path = "." // default to current dir
    }

    absPath, err := filepath.Abs(path)
    if err != nil {
        http.Error(w, "Invalid path", http.StatusBadRequest)
        return
    }

    entries, err := os.ReadDir(absPath)
    if err != nil {
        http.Error(w, err.Error(), http.StatusInternalServerError)
        return
    }

    var files []map[string]string
    for _, entry := range entries {
        entryPath := filepath.Join(absPath, entry.Name())
        absEntryPath, err := filepath.Abs(entryPath)
        if err != nil {
            continue
        }

        files = append(files, map[string]string{
            "name":  entry.Name(),
            "path":  absEntryPath,
            "isDir": strconv.FormatBool(entry.IsDir()),
        })
    }

    // Return both the file list and current absolute path
    json.NewEncoder(w).Encode(map[string]interface{}{
        "currentPath": absPath,
        "entries":     files,
    })
}
