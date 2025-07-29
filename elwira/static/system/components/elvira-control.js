import { TemplateComponent } from './template_component.js';

export class ElviraControl extends TemplateComponent {
  constructor() {
    super('./components/elvira-control.html');
    this.view = null;
    this.target = null;
    this.from_target = null;
    this.from_view = null;
  }

  onTemplateLoaded() {
    this.addEventListener('target-register', (event) => {
       console.log('elvira-control received target-register '+event.srcElement.tagName);
       event.stopImmediatePropagation();
       this.target = event.srcElement;
    });
    this.addEventListener('view-register', (event) => {
       console.log('elvira-control received view-register '+event.srcElement.params.name);
       event.stopImmediatePropagation();
       this.view = event.srcElement;
    });
    this.addEventListener('view-change', (event) => {
       console.log('elvira-control received view-change '+event.srcElement.params.name);
       event.stopImmediatePropagation();
       this.view = event.srcElement;
       this.from_view = event.detail;
       if (this.target != null) {
         this.target.update(this.from_view);
       } else {
          console.error('elvira-control no target registered');
       }
    });
    this.addEventListener('target-change', (event) => {
       console.log('elvira-control received target-change '+event.srcElement.tagName);
       event.stopImmediatePropagation();
       this.target = event.srcElement;
       this.from_target = event.detail;
       if (this.view != null && this.from_target.value != null) {
         this.view.update(this.from_target);
       }
       if (this.view == null) {
          console.error('elvira-control no view registered');
       }
    });
  }

}

customElements.define('elvira-control', ElviraControl);

