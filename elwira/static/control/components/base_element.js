// base-element.js
export class BaseElement extends HTMLElement {
  params = {points: [], name: 'anonymous'};
  constructor() {
    super();
    const template = this.constructor.template;
    const root = this.attachShadow({ mode: 'open' });

    if (template instanceof HTMLTemplateElement) {
      root.appendChild(template.content.cloneNode(true));
    } else {
      throw new Error(`${this.constructor.name}.template must be an HTMLTemplateElement`);
    }
    const options = this.querySelectorAll(":scope > option");
    if (options.length > 0) {
        let points = [];
        options.forEach(function (option, index) {
           points.push({pos: index, label: option.textContent, value: option.value});
        });
        this.params.points = points;
    }


  }

  // Combines static style + markup into a full <template>
  static get template() {
    const t = document.createElement('template');
    const css = typeof this.style === 'function' ? this.style() : (this.style ?? '');
    const html = typeof this.markup === 'function' ? this.markup() : (this.markup ?? '');
    t.innerHTML = css ? `<style>${css}</style>${html}` : html;
    return t;
  }
}
