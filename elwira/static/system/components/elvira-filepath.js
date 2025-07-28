import './file-selector-dialog.js';

import { ViewComponent } from './view_component.js';

export class ElviraFilepath extends ViewComponent {
  constructor() {
    super('./components/elvira-filepath.html');
    this.params = {
    }
    this.component_state = {};
  }


  onTemplateLoaded() {
    let x;
    if (x = this.getAttribute("name")) {this.params.name = x;}
    //console.log('Params', this.params);
    //this.component = this.shadowRoot.querySelector(".component");
    //const valueDisplay = this.shadowRoot.querySelector(".component-value");
    const nameDisplay = this.shadowRoot.querySelector(".component-name");
    nameDisplay.textContent = this.params.name;
    //this.reportState();
  }

/*
  reportState() {
     this.component.dispatchEvent(new CustomEvent("view-change", {detail: this.component_state, bubbles: true, composed: true} ));
  }
*/

 update(info) {
      console.warn('elvira-filepath update',info);
 }

}

customElements.define('elvira-filepath', ElviraFilepath);

