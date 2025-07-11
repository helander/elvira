let activeInput = null;

function setInputValue(input, value) {
    input.value = value;
    input.dispatchEvent(new Event("input", { bubbles: true }));
}


function openModal(input) {
    activeInput = input;
    document.getElementById("file-modal").classList.remove("hidden");
    loadDirectory(".");
}

function closeModal() {
    document.getElementById("file-modal").classList.add("hidden");
    document.getElementById("file-list").innerHTML = "";
    activeInput = null;
}

function loadDirectory(path) {
    fetch(`/listdir?path=${encodeURIComponent(path)}`)
        .then(res => res.json())
        .then(data => {
            const list = document.getElementById("file-list");
            const currentPath = document.getElementById("current-path");
            const absPath = data.currentPath;
            currentPath.textContent = absPath;
            list.innerHTML = "";

            // ✔ Select this folder
            const selectFolder = document.createElement("li");
            selectFolder.textContent = "✔ Select this folder";
            selectFolder.style.fontWeight = "bold";
            selectFolder.onclick = () => {
                if (activeInput) {
                    activeInput.value = absPath; // absolute from backend
                    setInputValue(activeInput, absPath);
                }
                closeModal();
            };
            list.appendChild(selectFolder);

            //  Go up
            if (absPath !== "/") {
                const up = document.createElement("li");
                up.textContent = " .. (parent)";
                up.onclick = () => loadDirectory(absPath + "/..");
                list.appendChild(up);
            }

            //  and  entries
            data.entries.forEach(entry => {
                const li = document.createElement("li");
                li.textContent = (entry.isDir === "true" ? " " : " ") + entry.name;
                if (entry.isDir === "true") {
                    li.onclick = () => loadDirectory(entry.path);
                } else {
                    li.onclick = () => {
                        if (activeInput) {
                            activeInput.value = entry.path;
                            setInputValue(activeInput, entry.path);
                        }
                        closeModal();
                    };
                }
                list.appendChild(li);
            });
        });
}
