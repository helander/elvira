/*
   File:           elvira.js
   Project:        elvira
   Author:         Lars-Erik Helander <lehswel@gmail.com>
   License:        MIT
   Description:    Control page support functions
*/

// =========================================================================================
// Run operations after millisec time delay
// =========================================================================================
function sleep(time) {
    return new Promise((resolve) => setTimeout(resolve, time));
}

// =========================================================================================
// Create new elvira instance
// =========================================================================================
function create_instance() {
    const plugin_uri = document.querySelector('#plugin-select');
    const instance_name = document.querySelector('#new-instance-name');
    const instance_showui = document.querySelector('#new-instance-showui');
    {
        const baseUrl = "/elvira";
        const params = new URLSearchParams({
            name: instance_name.value,
            uri: plugin_uri.value,
            showui: instance_showui.checked
        });
        fetch(`${baseUrl}?${params.toString()}`)
            .then(response => {
                if (!response.ok) throw new Error('Network response was not ok');
                sleep(2000).then(() => {
                    populate_instance_list();
                    fetch('/links').then(response => {});
                });
                return response.text();
            })
            .then(data => {})
            .catch(error => {
                console.error('presets Fetch error:', error);
            });
    }
}

// =========================================================================================
// Open control
// =========================================================================================
function openControl(node) {
    const url = "/control-inputs.html?node="+node;
    const windowName = "control"+node;
    win = window.open(url, windowName);
}

// =========================================================================================
// Open params
// =========================================================================================
function openParams(node) {
    const url = "/params.html?node="+node;
    const windowName = "params"+node;
    win = window.open(url, windowName);
}

// =========================================================================================
// Open log
// =========================================================================================
function openLog(pid) {
    const url = "/logs/"+pid;
    const windowName = "log"+pid;
    win = window.open(url, windowName);
}


// =========================================================================================
// Populate instance list
// =========================================================================================
function populate_instance_list() {
    fetch("/nodes")
        .then(response => {
            if (!response.ok) throw new Error('Network response was not ok');
            return response.json();
        })
        .then(data => {
            const tableBody = document.querySelector('#instance-table tbody');
            tableBody.innerHTML = '';
            if (data == null) data = [];
            data.sort((a, b) => {
               if (a.group < b.group) return -1;
               if (a.group > b.group) return 1;
               return a.step - b.step;
            });
            data.forEach(item => {
                if (item.gain == null) item.gain = 0;
                console.log('gain',item.gain);
                const row = document.createElement('tr');
                row.innerHTML = `
            <td>${item.group}</td>
            <td>${item.step}</td>
            <td>${item.id}</td>
            <td>${item.name}</td>
            <td>${item.plugin}</td>
            <td id="preset-${item.id}"></td>
            <td><a href="#" onclick="openLog(${item.pid})">Log</a></td>
            <td><a href="#" onclick="openControl(${item.id})">Controls</a></td>
            <td><a href="#" onclick="openParams(${item.id})">Params</a></td>
            <td><input type="range" step="0.01" id="volume-${item.id}"></a></td>
            <td><button onclick="show_save_preset_popup(this)" data-id="${item.id}">Save</button></td>
            <td><button onclick="delete_instance(${item.id},${item.pid})">Delete</button></td>
          `;
                const node_id = item.id;
                const minDb = -50;
                const maxDb = 20;
                const volval = (20*Math.log10(Number(item.gain)) - minDb)*100.0/(maxDb - minDb)

                tableBody.appendChild(row);
                  const volume = document.querySelector(`#volume-${item.id}`);
                  volume.value = volval;
                  volume.addEventListener('input', () => {
                     console.log('volval',volume.value);
                     const percent = Number(volume.value) / 100;
                     const db = minDb + (maxDb - minDb) * percent;
                     const gain = Math.pow(10, db / 20); 
                     const baseUrl = "/volume/"+node_id+"?gain="+gain;
                     console.log(baseUrl);
                     fetch(baseUrl);
                  })


                    const baseUrl = "/presets";
                    const params = new URLSearchParams({
                        uri: item.plugin
                    });
                    fetch(`${baseUrl}?${params.toString()}`)
                        .then(response => {
                            if (!response.ok) throw new Error('Network response was not ok');
                            return response.json();
                        })
                        .then(data => {
                            const presetTd = document.querySelector(`#preset-${item.id}`);
                            presetTd.innerHTML = '';
                            const presetSelect = document.createElement("select");
                            const empty = document.createElement('option');
                            empty.innerHTML = "";
                            presetSelect.appendChild(empty);
                            data.forEach(x => {
                                const opt = document.createElement('option');
                                opt.value = x.uri;
                                opt.text = x.label;
                                presetSelect.appendChild(opt);
                            });
                            for (let i = 0; i < presetSelect.options.length; i++) {
                                if (presetSelect.options[i].value === item.preset) {
                                    presetSelect.selectedIndex = i;
                                    break;
                                }
                            }
                            presetSelect.addEventListener('change', function(event) {
                                if (event.target.value != '') apply_preset(node_id, event.target.value);
                            });
                            presetTd.appendChild(presetSelect);
                        })
                        .catch(error => {
                            console.error('presets Fetch error:', error);
                        });
            });
        })
        .catch(error => {
            console.error('nodrs Fetch error:', error);
            const tableBody = document.querySelector('#instance-table tbody');
            tableBody.innerHTML = `<tr><td colspan="3">Failed to load data</td></tr>`;
        });

        const baseUrl = "/plugins";
        fetch(baseUrl)
            .then(response => {
                if (!response.ok) throw new Error('Network response was not ok');
                return response.json();
            })
            .then(data => {
                const pluginSelect = document.querySelector("#plugin-select");
                data.forEach(item => {
                    const opt = document.createElement('option');
                    opt.innerHTML = `${item}`;
                    pluginSelect.appendChild(opt);
                });
                pluginSelect.addEventListener('change', function(event) {
                    show_new_instance_popup();
                });
                pluginSelect.addEventListener('mousedown', function(event) {
                    show_new_instance_popup();
                });
            })
            .catch(error => {
                console.error('presets Fetch error:', error);
            });
}

// =========================================================================================
// Delete instance
// =========================================================================================
function delete_instance(node_id,pid) {
    fetch('/nodes/'+node_id+"?pid="+pid, {
            method: 'DELETE'
        })
        .then(response => {
            if (!response.ok) {
                throw new Error('Network response was not ok');
            }
            sleep(100).then(() => {
                populate_instance_list();
                fetch('/links').then(response => {});
           });
            return response.text();
        })
        .then(text => {
            //console.log(text);
        })
        .catch(error => {
            console.error('Fetch error:', error);
        });
}

// =========================================================================================
// Apply preset
// =========================================================================================
function apply_preset(node_id, preset_uri) {
    fetch('/metadata/'+node_id+'?key=use_preset&value='+preset_uri, {
            method: 'GET',
            //headers: {
            //    'Content-Type': 'application/json'
            //},
            //body: JSON.stringify({
            //    node_id: node_id,
            //    key: 'use_preset',
            //    value: preset_uri
            //})
        })
        .then(response => {
            if (!response.ok) {
                throw new Error('Network response was not ok');
            }
            return response.text();
        })
        .then(text => {
            //console.log(text);
        })
        .catch(error => {
            console.error('Fetch error:', error);
        });
}

// =========================================================================================
// Save preset
// =========================================================================================
function save_preset(node_id, preset_name) {
    fetch('/metadata/'+node_id+'?key=save_preset&value='+preset_name, {
            method: 'GET',
            //headers: {
            //    'Content-Type': 'application/json'
            //},
            //body: JSON.stringify({
            //    node_id: node_id,
            //    key: 'save_preset',
            //    value: preset_name
            //})
        })
        .then(response => {
            if (!response.ok) {
                throw new Error('Network response was not ok');
            }
            return response.text();
        })
        .then(text => {
            //console.log(text);
        })
        .catch(error => {
            console.error('Fetch error:', error);
        });
}

function submit_save_preset() {
    const name = document.getElementById('save-preset-name').value;
    save_preset(current_value, name);
    document.getElementById('save-preset-popup').style.display = 'none';
    document.getElementById('save-preset-name').value = ''; // Clear in
    sleep(1000).then(() => {
        populate_instance_list();
    });
}

function cancel_save_preset() {
    document.getElementById('save-preset-popup').style.display = 'none';
    document.getElementById('save-preset-name').value = ''; // Clear in
}

function show_save_preset_popup(button) {
    // Save the button-specific value
    current_value = button.getAttribute('data-id');
    // Show the popup
    document.getElementById('save-preset-popup').style.display = 'block';
}

// =========================================================================================
// New instance
// =========================================================================================
function submit_new_instance() {
    const name = document.getElementById('new-instance-name').value;
    create_instance();
    document.getElementById('new-instance-popup').style.display = 'none';
    document.getElementById('new-instance-name').value = ''; // Clear in
    sleep(2000).then(() => {
        populate_instance_list();
    });
}

function cancel_new_instance() {
    document.getElementById('new-instance-popup').style.display = 'none';
    document.getElementById('new-instance-name').value = ''; // Clear in
}

function show_new_instance_popup() {
    // Show the popup
    document.getElementById('new-instance-popup').style.display = 'block';
}

// =========================================================================================
// On load
// =========================================================================================
let current_value = null;
cancel_save_preset(); // Hide element
cancel_new_instance(); // Hide element
populate_instance_list();

fetch('/links').then(response => {});
