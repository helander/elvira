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
    this.addEventListener('file-selected', event => {
       console.log('filesel',event.detail);
       const filename = this.shadowRoot.querySelector("#filename");
       filename.innerHTML = shortenFilePath(event.detail.path).split('').map(c => `<span>${c}</span>`).join('');
    });
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


function shortenFilePath(path) {
  // Get the file name (after last slash)
  const fileName = path.split('/').pop();

  // Remove the extension (after last dot)
  const baseName = fileName.substring(0, fileName.lastIndexOf('.')) || fileName;

  return baseName;
}

customElements.define('elvira-filepath', ElviraFilepath);



