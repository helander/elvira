import { ViewComponent } from './view_component.js';

export class ElviraControl extends ViewComponent {
  constructor() {
    super('./components/elvira-control.html');
    this.target = null;
  }

  onTemplateLoaded() {
   requestAnimationFrame(() => {
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
    this.addEventListener('target-change', (event) => {
       //console.log('elvira-control received target-change');
       event.stopImmediatePropagation();
       if (this.target != null) {
         console.log('call target update method');
         this.view.update(event.detail);
       } else {
         console.error('No target bound yet !!!');
       }
    });
    this._bind();
   });
  }

  _bind() {
    //console.error('BIND '+this.tagName);
    const elements = Array.from(this.children);
    this.target = elements.find(el => el.classList.contains('target'));
    this.view = elements.find(el => el.classList.contains('view'));

    if (!this.view) {
      console.warn('Missing view element in <elvira-control>');
      return;
    }

    if (this.target &&  typeof this.target.fromTarget === 'function') {
      this.target.fromTarget();
    } else {
	 console.error('target or target.fromTarget is missing');
    }
  }
}

customElements.define('elvira-control', ElviraControl);

