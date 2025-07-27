// view_component.js
import { loadTemplate } from './template_loader.js';

export class ViewComponent extends HTMLElement {
  constructor(templateUrl) {
    super();
    this.attachShadow({ mode: 'open' });
    this._templateUrl = templateUrl;
    //this.component_role = 'view';
    this.params = {};
    //console.error('construct '+this.tagName);
  }

  async connectedCallback() {
    this.classList.add('view');
    //console.error('connectedCallback begin '+this.tagName);
    const template = await loadTemplate(this._templateUrl);
    this.shadowRoot.appendChild(template.content.cloneNode(true));
    let x;
    if (x = this.getAttribute("max")) {this.params.max = x;}
    if (x = this.getAttribute("min")) {this.params.min = x;}
    if (x = this.getAttribute("value")) {this.params.value = x;}
    if (this.getAttribute("integer") == null) {this.params.integer = false;}
    if (x = this.getAttribute("prio")) {this.params.prio = x;}
    if (x = this.getAttribute("name")) {this.params.name = x;}
    const options = this.querySelectorAll("option");
    if (options.length > 0) {
        let points = [];
        options.forEach(function (option, index) {
           points.push({pos: index, label: option.textContent, value: option.value});
        });
        this.params.points = points;
    }

    this.onTemplateLoaded();
    //console.error('connectedCallback end '+this.tagName);

  }

  // Optionally overridden in child classes
  onTemplateLoaded() {}

  update(info) {
      console.error('Missing update method in view sub class');
  }
}
