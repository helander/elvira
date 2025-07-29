// template_component.js
import { loadTemplate } from './template_loader.js';

export class TemplateComponent extends HTMLElement {
  constructor(templateUrl) {
    super();
    this.attachShadow({ mode: 'open' });
    this._templateUrl = templateUrl;
  }

  async connectedCallback() {
    console.error('connectedCallback begin '+this.tagName);
    const template = await loadTemplate(this._templateUrl);
    this.shadowRoot.appendChild(template.content.cloneNode(true));
    this.onTemplateLoaded();
  }

  // Overridden in child classes
  onTemplateLoaded() {}

}
