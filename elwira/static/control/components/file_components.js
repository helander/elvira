// file_components.js

import { BaseElement } from './base_element.js';



///////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Element tag: <file-browser-dialog>
//   Element class: FileBrowserDialog
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
export class FileBrowserDialog extends BaseElement {
  static style = `
    :host {
      display: none;
      position: fixed;
      top: 0; left: 0; right: 0; bottom: 0;
      background: rgba(0,0,0,0.5);
      justify-content: center;
      align-items: center;
      z-index: 9999;
    }
    :host([visible]) {
      display: flex;
    }
    .dialog {
      background: white;
      padding: 1rem;
      border-radius: 6px;
      min-width: 320px;
      max-width: 500px;
      max-height: 80vh;
      overflow: auto;
      box-shadow: 0 0 10px rgba(0,0,0,0.3);
    }
    ul {
      list-style: none;
      padding: 0;
      margin: 0;
      max-height: 300px;
      overflow-y: auto;
      border: 1px solid #ccc;
    }
    li {
      padding: 0.4rem 0.5rem;
      cursor: pointer;
      border-bottom: 1px solid #eee;
      display: flex;
      align-items: center;
      justify-content: space-between;
    }
    li:hover {
      background-color: #f0f0f0;
    }
    li.selected {
      background-color: #d0e0ff;
    }
    li.folder::after {
      content: "📁";
      margin-left: 0.5rem;
    }
    li.file::after {
      content: "📄";
      margin-left: 0.5rem;
    }
    footer {
      text-align: right;
      margin-top: 1rem;
    }
    button {
      margin-left: 0.5rem;
    }
  `;

  static markup = `
     <div class="dialog">
         <h3>Select a file or folder</h3>
         <div><strong id="current-path">/</strong></div>
         <ul id="file-list"></ul>
         <footer>
           <button id="cancel">Cancel</button>
           <button id="ok" disabled>OK</button>
         </footer>
     </div>
  `;

  constructor() {
    super();
    this.selectedPath = null;
    this.selectedIsDir = false;
    this.currentPath = '/';
    const list = this.shadowRoot.getElementById('file-list');
    const okBtn = this.shadowRoot.getElementById('ok');

    list.addEventListener('click', (e) => {
      const li = e.target.closest('li');
      if (!li) return;

      if (li.dataset.type === 'up') {
        this.goUp();
        return;
      }

      const isDir = li.dataset.isdir === "true";
      const path = li.dataset.path;

      if (isDir) {
        this.open(path);
      } else {
        this.selectItem(path, false);
      }
    });

    okBtn.addEventListener('click', () => {
      if (this.selectedPath) {
        this.dispatchEvent(new CustomEvent('file-found', {
          detail: {
            path: this.selectedPath,
            isDir: this.selectedIsDir
          },
          bubbles: true,
          composed: true
        }));
        this.close();
      }
    });

    this.shadowRoot.getElementById('cancel').addEventListener('click', () => {
      this.close();
    });
  }

  async open(path = '/') {
    this.currentPath = path;
    this.clearSelection();
    await this.loadFileList(path);
    this.setAttribute('visible', '');
  }

  close() {
    this.removeAttribute('visible');
  }

  clearSelection() {
    this.selectedPath = null;
    this.selectedIsDir = false;
    this.shadowRoot.querySelectorAll('li').forEach(li => li.classList.remove('selected'));
    this.shadowRoot.getElementById('ok').disabled = true;
  }

  selectItem(path, isDir) {
    this.selectedPath = path;
    this.selectedIsDir = isDir;
    this.shadowRoot.querySelectorAll('li').forEach(li => {
      li.classList.toggle('selected', li.dataset.path === path);
    });
    this.shadowRoot.getElementById('ok').disabled = false;
  }

  async goUp() {
    const segments = this.currentPath.split('/').filter(Boolean);
    segments.pop();
    const upPath = '/' + segments.join('/');
    await this.open(upPath || '/');
  }

  async loadFileList(path) {
    const list = this.shadowRoot.getElementById('file-list');
    const pathLabel = this.shadowRoot.getElementById('current-path');
    list.innerHTML = '<li><em>Loading...</em></li>';
    pathLabel.textContent = path;

    try {
      const res = await fetch(`/listdir?path=${encodeURIComponent(path)}`);
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      const json = await res.json();

      list.innerHTML = '';
      if (path !== '/') {
        const up = document.createElement('li');
        up.textContent = '..';
        up.dataset.type = 'up';
        up.style.fontStyle = 'italic';
        list.appendChild(up);
      }

      for (const entry of json.entries) {
        const li = document.createElement('li');
        li.textContent = entry.name;
        li.dataset.name = entry.name;
        li.dataset.path = entry.path;
        li.dataset.isdir = entry.isDir;
        li.classList.add(entry.isDir === "true" ? 'folder' : 'file');
        list.appendChild(li);
      }
    } catch (err) {
      list.innerHTML = `<li style="color:red;"><em>Error: ${err.message}</em></li>`;
    }
  }
}

customElements.define('file-browser-dialog', FileBrowserDialog);

///////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Element tag: <file-path-input>
//   Element class: FilePathInput
//
///////////////////////////////////////////////////////////////////////////////////////////////////////

export class FilePathInput extends BaseElement {

  static style = `
    input { width: 300px; margin-right: 0.5rem; }
    input { z-index: 500;}
    button { z-index: 500;}
  `;

  static markup = `
    <input type="text" id="path" placeholder="Enter file or folder path" />
    <button id="browse">Browse...</button>
  `;


  constructor() {
    super();
    this.shadowRoot.getElementById('browse').addEventListener('click', () => {
      this.dispatchEvent(new CustomEvent('open-file-browser', {
        detail: { path: '/' },
        bubbles: true,
        composed: true
      }));
    });
  }

  setPath(path) {
    this.shadowRoot.getElementById('path').value = path;
  }
}

customElements.define('file-path-input', FilePathInput);

///////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Element tag: <file-selector-dialog>
//   Element class: FileSelectorDialog
//
///////////////////////////////////////////////////////////////////////////////////////////////////////

export class FileSelectorDialog extends BaseElement {

  static style = `
    #modal {
          display: none;
          position: fixed;
          top: 0; left: 0; right: 0; bottom: 0;
          background: rgba(0,0,0,0.5);
          justify-content: center;
          align-items: center;
          z-index: 9999;
          pointer-events: none; /* Prevent interaction when closed */
    }
    #modal[open] {
          display: flex;
          pointer-events: auto; /* Reactivate pointer when visible */
    }
    .dialog {
          background: white;
          padding: 1rem;
          border-radius: 6px;
          min-width: 350px;
          box-shadow: 0 0 10px rgba(0,0,0,0.3);
    }
    footer {
          text-align: right;
          margin-top: 1rem;
    }
    #open-dialog {
       font-family: sans-serif;
       font-weight: bold;
       color: grey;
       border-color: grey;
       border-radius: 6px;
       box-shadow: inset 0 0 4px #000;
       text-align: center;
       line-height: 20px;
       vertical-align: middle;
       font-size: 10px;
    }
  `;

  static markup = `
      <button id="open-dialog">File</button>
      <div id="modal">
        <div class="dialog">
          <file-path-input></file-path-input>
          <footer>
            <button id="cancel">Cancel</button>
            <button id="ok">OK</button>
          </footer>
        </div>
      </div>
      <file-browser-dialog></file-browser-dialog>
  `;

  constructor() {
    super();
    this.selectedPath = '';
    const openBtn = this.shadowRoot.getElementById('open-dialog');
    const modal = this.shadowRoot.getElementById('modal');
    const cancelBtn = this.shadowRoot.getElementById('cancel');
    const okBtn = this.shadowRoot.getElementById('ok');
    const dialog = this.shadowRoot.querySelector('file-browser-dialog');
    const input = this.shadowRoot.querySelector('file-path-input');

    openBtn.addEventListener('click', () => {
      modal.setAttribute('open', '');
    });

    cancelBtn.addEventListener('click', () => {
      modal.removeAttribute('open');
    });

    okBtn.addEventListener('click', () => {
      const path = input.shadowRoot.getElementById('path').value;
      this.selectedPath = path;
      modal.removeAttribute('open');
      this.dispatchEvent(new CustomEvent('file-selected', {
          detail: {
            path: this.selectedPath
          },
          bubbles: true,
          composed: true
      }));
    });

    // Forward file browser open request to dialog
    this.addEventListener('open-file-browser', () => {
      dialog.open('/');
    });

    // Handle file selection from dialog and forward to input
    this.addEventListener('file-found', e => {
      input.setPath(e.detail.path);
    });
  }
}

customElements.define('file-selector-dialog', FileSelectorDialog);
