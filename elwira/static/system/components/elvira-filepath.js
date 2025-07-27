import './file-selector-dialog.js';

import { ViewComponent } from './view_component.js';

export class ElviraFilepath extends ViewComponent {
  constructor() {
    super('./components/elvira-filepath.html');
    this.params = {
//      prio: 0,
//      max: 100,
//      min: 0,
//      integer: true,
      name: 'anonymous',
//      value: 37,
//      points: [],
//      pos: 0,
    }
    this.component_state = {};
  }


  onTemplateLoaded() {
    let x;
    //if (x = this.getAttribute("pos")) {this.params.pos = x;}
    //if (x = this.getAttribute("min")) {this.params.min = x;}
    //if (x = this.getAttribute("value")) {this.params.value = x;}
    //if (this.getAttribute("integer") == null) {this.params.integer = false;}
    //if (x = this.getAttribute("prio")) {this.params.prio = x;}
    if (x = this.getAttribute("name")) {this.params.name = x;}
    //console.log('integer',this.getAttribute("integer"));
    console.log('Params', this.params);
    //this.component = this.shadowRoot.querySelector(".component");
    //const valueDisplay = this.shadowRoot.querySelector(".component-value");
    const nameDisplay = this.shadowRoot.querySelector(".component-name");
    nameDisplay.textContent = this.params.name;
    //this.reportState();
  }

/*
  reportState() {
     this.component.dispatchEvent(new CustomEvent("component-change", {detail: this.component_state, bubbles: true, composed: true} ));
  }
*/

 update(info) {
      console.warn('elvira-filepath update',info);
 }

}

customElements.define('elvira-filepath', ElviraFilepath);

