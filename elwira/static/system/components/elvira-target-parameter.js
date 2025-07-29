import { TargetComponent } from './target_component.js';

export class ElviraTargetParameter extends TargetComponent {
  constructor() {
    super();
    this.params = {
    }
  }

  async onStart() {
    //console.log('elvira-target-parameter node',this.elvira_node);
    this.uri = this.getAttribute("uri");
    let detail = {};
     /* not yet any support for getting current parameter values

     const serviceUrl = "/node-props/"+this.elvira_node+"?prefix=elvira.control.in";
     const response = await fetch(serviceUrl);
     const data = await response.json();
     const current = data.properties["elvira.control.in."+this.index];
     //console.log('current',current);
     detail.value = current;
     */
     this.dispatchEvent(new CustomEvent('target-change', {
        detail: detail,
        bubbles: true,
        composed: true
     }));
  }

  async update(info) {
     console.log('parameter update target',info);
     if (this.uri != null) {
       const serviceUrl = "/metadata/"+this.elvira_node+"?key="+encodeURIComponent(this.uri)+"&value="+encodeURIComponent(info.value);
       console.log(serviceUrl);
       const response = await fetch(serviceUrl);
     } else {
        console.error("Missing uri attribute in <elvira-target-parameter>");
     }
  }
}

customElements.define('elvira-target-parameter', ElviraTargetParameter);
