// view_component.js
import { TemplateComponent } from './template_component.js';

export class ViewComponent extends TemplateComponent {
  constructor(templateUrl) {
    super(templateUrl);
    this.params = {};
  }

  onTemplateLoaded() {

  Promise.resolve().then(() => {
    requestAnimationFrame(() => {
      this.dispatchEvent(new CustomEvent('view-register', {
        detail: {},
        bubbles: true,
        composed: true
      }));
    });
  });


    this.classList.add('view');
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

    this.onStart();
    //console.error('connectedCallback end '+this.tagName);

  }

  // Optionally overridden in child classes
  onStart() {}

  update(info) {
      console.error('Missing update method in view sub class');
  }
}
