import { ViewComponent } from './view_component.js';

export class ElviraSelect extends ViewComponent {
  constructor() {
    super('./components/elvira-select.html');
    this.params = {
      prio: 0,
      max: 100,
      min: 0,
      integer: true,
      name: 'anonymous',
      value: 37,
      points: [],
      pos: 0,
    }
    this.component_state = {};
  }

  onTemplateLoaded() {
    let pos;
    if (pos = this.getAttribute("pos")) {this.params.pos = pos;}
    console.log('Params', this.params);
    this.component = this.shadowRoot.querySelector(".component");
    this.createComponent(this.params);
    this.shadowRoot.appendChild(this.component);
    this.reportState();
  }

  reportState() {
     this.component.dispatchEvent(new CustomEvent("view-change", {detail: this.component_state, bubbles: true, composed: true} ));
  }

  createComponent(params) {
       this.component_state = { name: params.name, pos: params.pos, value: params.points[params.pos].value };

      // Lookup elements in the shadow tree (Defined in the template)
      const buttonsContainer = this.shadowRoot.querySelector(".buttons-container");
      const valueDisplay = this.shadowRoot.querySelector(".component-value");
      const nameDisplay = this.shadowRoot.querySelector(".component-name");

      nameDisplay.textContent = params.name;

      const groupName = "this-group";

      for (let i = params.points.length - 1; i >= 0; i--) {
         const point = params.points[i];
         const label = document.createElement("label");
         label.className = "radio-button";
         label.textContent = point.label;

         const input = document.createElement("input");
         input.id = "rb"+i;
         input.type = "radio";
         input.name = groupName;
         input.value = i;
         if (i == params.pos) input.checked = true;

         label.appendChild(input);
         buttonsContainer.appendChild(label);
      }

      buttonsContainer.addEventListener("change", (event) => {
        if (event.target.name === groupName) {
           let val = event.target.value;
           this.component_state = { name: params.name, pos: val, value: params.points[val].value };
           this.reportState();
        }
      });
  }

  update(info) {
      //console.warn('elvira-select update',info);
      for (let i = 0; i < this.params.points.length; i++) {
        let point = this.params.points[i];
        if (point.value == info.value) {
           this.component_state.pos = i;
           this.component_state.value = point.value;
           const input = this.shadowRoot.querySelector("#rb"+i);
           input.checked = true;
           console.log(input);
           return;
        } 
      }
  }

}

customElements.define('elvira-select', ElviraSelect);
