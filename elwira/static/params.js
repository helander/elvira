
document.addEventListener("DOMContentLoaded", () => {
    const form = document.getElementById("form-container");

    const fields = [
        { label: "Username", path: false },
        { label: "Log File", path: true },
        { label: "Output Folder", path: true }
    ];

    fields.forEach((field, idx) => {
        const div = document.createElement("div");

        const input = document.createElement("input");
        input.type = "text";
        input.id = `input-${idx}`;

        div.appendChild(document.createTextNode(field.label + ": "));
        div.appendChild(input);

        if (field.path) {
            const button = document.createElement("button");
            button.textContent = "Browse...";
            button.onclick = () => openModal(input);
            div.appendChild(button);
        }

        form.appendChild(div);
    });
});

