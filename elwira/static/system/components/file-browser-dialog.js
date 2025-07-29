import { TemplateComponent } from './template_component.js';

export class FileBrowserDialog extends TemplateComponent {
  constructor() {
    super('./components/file-browser-dialog.html');
    this.selectedPath = null;
    this.selectedIsDir = false;
    this.currentPath = '/';
  }

  onTemplateLoaded() {
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
