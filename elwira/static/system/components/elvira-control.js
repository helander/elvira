import { ViewComponent } from './view_component.js';

export class ElviraControl extends ViewComponent {
  constructor() {
    super('./components/elvira-control.html');
    this.target = null;
  }

  onTemplateLoaded() {
    const slot = this.shadowRoot.querySelector('slot');
    slot.addEventListener('slotchange', () => this._bind());
    this.addEventListener('component-change', (event) => {
       //console.log('elvira-control received component-change');
       event.stopImmediatePropagation();
       if (this.target != null) {
         //console.log('call target update method');
         this.target.update(event.detail);
       } else {
         console.error('No target bound yet !!!');
       }
    });
    this._bind();
  }

  _bind() {
    const elements = Array.from(this.children);
    this.target = elements.find(el => el.component_role == 'target');
    this.view = elements.find(el => el.component_role == 'view');

    if (!this.view) {
      console.warn('Missing view element in <elvira-control>');
      return;
    }

    //if (target &&  typeof target.fetch === 'function') {
    //  target.fetch();
    //}
  }
}

customElements.define('elvira-control', ElviraControl);

