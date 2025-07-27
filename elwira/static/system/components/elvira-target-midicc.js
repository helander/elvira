import { TargetComponent } from './target_component.js';

export class ElviraTargetMidicc extends TargetComponent {
  constructor() {
    super();
    this.params = {
    }
  }

  onStart() {
    this.endpoint = this.getAttribute('endpoint') || '/default';
    console.log('elvira-data-midicc node',this.elvira_node);
    this.midicc = this.getAttribute("midicc");
   }

  async fromTarget() {
     const serviceUrl = "/node-props/"+this.elvira_node+"?prefix=elvira.midicc";
     const response = await fetch(serviceUrl);
     const data = await response.json();
     const current = data.properties["elvira.midicc."+this.midicc];
     this.dispatchEvent(new CustomEvent('target-change', {
        detail: {value: current},
        bubbles: true,
        composed: true
     }));
  }

  update(info) {
     //console.log('midicc update',info);
     if (this.midicc != null) {
       const serviceUrl = "/metadata/"+this.elvira_node+"?key=midicc."+this.midicc+"&value="+info.value;
       console.log(serviceUrl);
       fetch(serviceUrl);
     } else {
        console.error("Missing midicc attribute in <elvira-target-midicc>");
     }
  }
}

customElements.define('elvira-target-midicc', ElviraTargetMidicc);




