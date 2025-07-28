import './file-path-input.js';
import './file-browser-dialog.js';

import { ViewComponent } from './view_component.js';

export class FileSelectorDialog extends ViewComponent {
  constructor() {
    super('./components/file-selector-dialog.html');
    this.selectedPath = '';
  }

  onTemplateLoaded() {
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
      console.log("Selected path:", path);
    });

    // Forward file browser open request to dialog
    this.addEventListener('open-file-browser', () => {
      dialog.open('/');
    });

    // Handle file selection from dialog and forward to input
    this.addEventListener('file-selected', e => {
      input.setPath(e.detail.path);
    });
  }
}

customElements.define('file-selector-dialog', FileSelectorDialog);
