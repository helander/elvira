import { ViewComponent } from './view_component.js';

export class ElviraControl extends ViewComponent {
  constructor() {
    super('./components/elvira-control.html');
    this.view = null;
    this.target = null;
    this.from_target = null;
    this.from_view = null;
  }

  onTemplateLoaded() {
    this.addEventListener('view-change', (event) => {
       console.log('elvira-control received view-change',event);
       event.stopImmediatePropagation();
       this.view = event.srcElement;
       this.from_view = event.detail;
       if (this.target != null) {
         this.target.update(this.from_view);
       }
    });
    this.addEventListener('target-change', (event) => {
       console.log('elvira-control received target-change');
       event.stopImmediatePropagation();
       this.target = event.srcElement;
       this.from_target = event.detail;
       if (this.view != null) {
         this.view.update(this.from_target);
       }
    });
  }

}

customElements.define('elvira-control', ElviraControl);

