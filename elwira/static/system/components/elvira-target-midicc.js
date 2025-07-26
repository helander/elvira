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

  fetch() {
    setTimeout(() => {
      const fakeData = {
        endpoint: this.endpoint,
        values: [5, 10, 15, 20]
      };
      this.dispatchEvent(new CustomEvent('target-change', {
        detail: fakeData,
        bubbles: true,
        composed: true
      }));
    }, 500);
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


