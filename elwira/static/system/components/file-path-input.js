import { TemplateComponent } from './template_component.js';

export class FilePathInput extends TemplateComponent {
  constructor() {
    super('./components/file-path-input.html');
  }

  onTemplateLoaded() {
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
