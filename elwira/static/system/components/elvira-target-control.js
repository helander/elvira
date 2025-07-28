import { TargetComponent } from './target_component.js';

export class ElviraTargetControl extends TargetComponent {
  constructor() {
    super();
    this.params = {
    }
  }

  async onStart() {
    //console.log('elvira-target-control node',this.elvira_node);
    this.index = this.getAttribute("index");
     const serviceUrl = "/node-props/"+this.elvira_node+"?prefix=elvira.control.in";
     const response = await fetch(serviceUrl);
     const data = await response.json();
     const current = data.properties["elvira.control.in."+this.index];
     //console.log('current',current);
     this.dispatchEvent(new CustomEvent('target-change', {
        detail: {value: current},
        bubbles: true,
        composed: true
     }));
  }

  async update(info) {
     //console.log('control update target',info);
     if (this.index != null) {
       const serviceUrl = "/metadata/"+this.elvira_node+"?key=control.in."+this.index+"&value="+info.value;
       //console.log(serviceUrl);
       const response = await fetch(serviceUrl);
     } else {
        console.error("Missing index attribute in <elvira-target-control>");
     }
  }
}

customElements.define('elvira-target-control', ElviraTargetControl);
