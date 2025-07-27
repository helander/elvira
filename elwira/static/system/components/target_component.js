// target_component.js

export class TargetComponent extends HTMLElement {
  constructor() {
    super();
    //this.component_role = 'target';
    //console.error('constructor '+this.tagName);
  }

 async connectedCallback() {
    this.classList.add('target');;
    //console.error('connectedCallback begin '+this.tagName);
     this.elvira_node = this.getAttribute("node");
     if (this.elvira_node === null) {
       const found = this.findAncestorAttribute('node');
       if (found !== null) {
          this.elvira_node = found;
       } else {
         const params = new URLSearchParams(window.location.search);
         const node = params.get('node');
         if (node !== null) {
             this.elvira_node = node;
         } else {
             console.error('Url parameter node not found!!! ');
         }
       }
     }
     this.onStart();
    //console.error('connectedCallback end '+this.tagName);
  }

  // Optionally overridden in child classes
  onStart() {}

  findAncestorAttribute(attrName) {
      let el = this;

      while (el) {
        if (el.getAttribute(attrName)) {
          return el.getAttribute(attrName);
        }

        const root = el.getRootNode();
        if (root instanceof ShadowRoot) {
          el = root.host; // Climb out of shadow DOM
        } else {
          el = el.parentNode; // Continue in light DOM
        }
        if (el === document.documentElement) return null;
      }

      return null; // Attribute not found
  }

  update() {
    console.error('No update method defined in subclass');
  }
}
