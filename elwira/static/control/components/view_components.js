import { BaseElement } from './base_element.js';
import './file_components.js';

function getNameValue(start) {
  let el = start;
  while (el) {
    if (el.hasAttribute?.('name')) {
      return el.getAttribute('name');
    }
    el = el.parentNode || el.host;  // climb up light + shadow DOM
  }

  const params = new URLSearchParams(window.location.search);
  return params.get('name');
}



export class ElviraControl extends BaseElement {

  static style = `
    :host {
       --surface-height: 300px;
    }
    ::slotted(.target) {
      display: none !important;
    }
  `;

  static markup = `
     <slot></slot>
  `;


      constructor() {
        super();
        this.bus = new EventTarget();
        const slot = this.shadowRoot.querySelector('slot');
        const process = () => {
          const assigned = slot.assignedElements();
          for (const el of assigned) {
            const tag = el.tagName.toLowerCase();
            customElements.whenDefined(tag).then(() => {
              if (typeof el.setBus === 'function') {
                el.setBus(this.bus);
              }
            });
          }
        };
        this.addEventListener('component-changed', (e) => {
          //console.log('catched component-changed',e);
          e.stopImmediatePropagation();
          if (this.bus) { 
            this.bus.dispatchEvent(new CustomEvent('view-changed', {
              detail: e.detail
            }));
          } else {
            console.error('No bus available - possible race condition');
          }
        });
        slot.addEventListener('slotchange', process);
        queueMicrotask(process);
      }
}

customElements.define('elvira-control', ElviraControl);



export class FilepathView extends BaseElement {
  static style = `

    .component {
      display: flex;
      align-items: center;
      margin: 0px;
      gap: 10px;
      flex-direction: column;
    }

    .component-name {
      margin-top: 25px;
      font-size: 13px;
      width: 10ch;
      text-align: center;
      line-height: 1em;
      min-height: 2em;
      max-height: 2em;
      overflow: hidden;
      white-space: normal;
      word-break: break-word;
      display: block;
    }

    .component-value {
      margin-bottom: 25px;
      font-weight: bold;
      font-size: 14px;
      text-align: center;
      white-space: nowrap;
      min-width: 10ch;
      min-height: 1em;
      max-height: 1em;

      overflow: hidden;
      text-overflow: ellipsis;
    }

    .component-surface {
      display: flex;
      flex-direction: column;
      width: 100%;
      height: var(--surface-height,300px);
      position: relative;
      margin: 0px;
    }

    .stacked-text {
      height: 100%;
      overflow: hidden;
      line-height: 1;
      font-size: 11px;
    }
    .stacked-text span {
      display: block;
      text-align: center;
      font-weight: bold;
    }
  `;

  static markup = `
   <div class="component">
       <div class="component-value"></div>
       <div class="component-surface">
          <file-selector-dialog id="selector"></file-selector-dialog>
          <div class="stacked-text" id="filename">
              <!-- JS inserts one span per character -->
          </div>
       </div>
       <div class="component-name"></div>
   </div>
  `;

  constructor() {
    super();
    //this.params = {}
    this.component_state = {};
    this.addEventListener('file-selected', event => {
       const filename = this.shadowRoot.querySelector("#filename");
       filename.innerHTML = shortenFilePath(event.detail.path).split('').map(c => `<span>${c}</span>`).join('');
       //Promise.resolve().then(() => {
       //   requestAnimationFrame(() => {
            this.component.dispatchEvent(new CustomEvent('component-changed', {
              detail: {value: event.detail.path},
              bubbles: true,
              composed: true
            }));
       //   });
       //});
    });
  }

  connectedCallback() {
     const name = getNameValue(this);
     if (name != null) this.params.name = name;
     this.component = this.shadowRoot.querySelector(".component");
     const nameDisplay = this.shadowRoot.querySelector(".component-name");
     nameDisplay.textContent = this.params.name;
  }

}

function shortenFilePath(path) {
  const fileName = path.split('/').pop();
  const baseName = fileName.substring(0, fileName.lastIndexOf('.')) || fileName;
  return baseName;
}

customElements.define('filepath-view', FilepathView);



export class SelectView extends BaseElement {
  static style = `

    .component {
      display: flex;
      align-items: center;
      margin: 0px;
      gap: 10px;

      flex-direction: column;                                                                                                                                                                       
    }

    .component-name {
      margin-top: 25px;
      font-size: 13px;
      width: 10ch;
      text-align: center;
      line-height: 1em;
      min-height: 2em;
      max-height: 2em;
      overflow: hidden;
      white-space: normal;
      word-break: break-word;
      display: block;
    }

    .component-value {
      margin-bottom: 25px;
      font-weight: bold;
      font-size: 14px;
      text-align: center;
      white-space: nowrap;
      min-width: 10ch;
      min-height: 1em;
      max-height: 1em;

      overflow: hidden;
      text-overflow: ellipsis;
    }

    .component-surface {
      display: flex;
      flex-direction: column;
      width: 100%;
      height: var(--surface-height,300px);
      position: relative;
      margin: 0px;
    }

    .buttons-container {
      display: flex;
      flex-direction: column;
      /* justify-content: space-around; */
      /* height: 300px;*/
      font-size: 12px;
      user-select: none;
      width: 90%;
      position: relative;
      border: 1px solid grey;
      border-radius: 6px;
    }



.radio-button {
  display: block;
  width: 50px;
  height: 20px;
  margin: 5px auto;
  padding: 0;
  font-family: sans-serif;
  font-weight: bold;
  color: grey;
  border-color: grey;
  /*background-image: url('https://www.publicdomainpictures.net/pictures/30000/velka/brushed-metal-background.jpg');*/
  background-size: cover;
  background-position: center;
  border: 2px solid #444;
  border-radius: 6px;
  box-shadow: inset 0 0 4px #000;
  text-align: center;
  line-height: 20px;
  vertical-align: middle;
  cursor: pointer;
  position: relative;
  transition: box-shadow 0.3s, border-color 0.3s;
  font-size: 10px;
}

.radio-button input {
  display: none;
}

.radio-button:has(input:checked) {
  box-shadow: 0 0 12px 4px black, inset 0 0 6px black;
  border-color: black; /* #0f0 */
  color: black;
}
  `;


  static markup = `
   <!-- Component inner structure -->
   <div class="component">
       <div class="component-value"></div>
       <div class="component-surface">
          <div class="buttons-container"></div>
       </div>
       <div class="component-name"></div>
   </div>
  `;


  constructor() {
    super();
    this.params.pos = 0;
    this.component_state = {};
  }

  connectedCallback() {
     const name = getNameValue(this);
     if (name != null) this.params.name = name;
     let attr;
     if (attr = this.getAttribute("pos")) {this.params.pos = attr;}
     this.component = this.shadowRoot.querySelector(".component");
     const nameDisplay = this.shadowRoot.querySelector(".component-name");
     nameDisplay.textContent = this.params.name;
     this.component = this.shadowRoot.querySelector(".component");
     this.createComponent(this.params);
     this.shadowRoot.appendChild(this.component);
     if (this.params.pos != null) {
        this.reportState();
     }
  }


  reportState() {
   //Promise.resolve().then(() => {
   // requestAnimationFrame(() => {
      this.component.dispatchEvent(new CustomEvent('component-changed', {
        detail: this.component_state,
        bubbles: true,
        composed: true
      }));
   // });
   //});
  }

  createComponent(params) {
       this.component_state = { name: params.name, pos: params.pos, value: params.points[params.pos].value };

      // Lookup elements in the shadow tree (Defined in the template)
      const buttonsContainer = this.shadowRoot.querySelector(".buttons-container");
      const valueDisplay = this.shadowRoot.querySelector(".component-value");
      const nameDisplay = this.shadowRoot.querySelector(".component-name");

      nameDisplay.textContent = params.name;

      const groupName = "this-group";

      for (let i = params.points.length - 1; i >= 0; i--) {
         const point = params.points[i];
         const label = document.createElement("label");
         label.className = "radio-button";
         label.textContent = point.label;

         const input = document.createElement("input");
         input.id = "rb"+i;
         input.type = "radio";
         input.name = groupName;
         input.value = i;
         if (i == params.pos) input.checked = true;

         label.appendChild(input);
         buttonsContainer.appendChild(label);
      }

      buttonsContainer.addEventListener("change", (event) => {
        if (event.target.name === groupName) {
           let val = event.target.value;
           this.component_state = { name: params.name, pos: val, value: params.points[val].value };
           this.reportState();
        }
      });
  }

      setBus(bus) {
        if (this.bus == bus) return;
        this.bus = bus;

        this.bus.addEventListener('data-ready', e => {
          //console.log('catched data-ready',e);
          e.stopImmediatePropagation();
          if (this.params.value == null) {
            this.update(e.detail.value);
          }
        });
      }

  update(value) {
      //console.warn('elvira-select update',info);
      for (let i = 0; i < this.params.points.length; i++) {
        let point = this.params.points[i];
        if (point.value == value) {
           this.component_state.pos = i;
           this.component_state.value = point.value;
           const input = this.shadowRoot.querySelector("#rb"+i);
           input.checked = true;
           return;
        } 
      }
  }

}

customElements.define('select-view', SelectView);



/*
TODO: make current available semantics selectable via attributes
      - snapping to label positions
      - float or integer
      - show label or value in value field
      - setting attributes to  snapping and or showing labels in value field should produce error if no points (<option>:s) defined
*/


export class SliderView extends BaseElement {
  static style = `

    .component {
      display: flex;
      align-items: center;
      margin: 0px;
      gap: 10px;

      flex-direction: column;                                                                                                                                                                       
    }

    .component-name {
      margin-top: 25px;
      font-size: 13px;
      width: 10ch;
      text-align: center;
      line-height: 1em;
      min-height: 2em;
      max-height: 2em;
      overflow: hidden;
      white-space: normal;
      word-break: break-word;
      display: block;
    }

    .component-value {
      margin-bottom: 25px;
      font-weight: bold;
      font-size: 14px;
      text-align: center;
      white-space: nowrap;
      min-width: 10ch;
      min-height: 1em;
      max-height: 1em;

      overflow: hidden;
      text-overflow: ellipsis;
    }

     .slider-container {                                                                                                                                                                          
        display: grid;                                                                                                                                                                             
        width: 100%;
     }

     .slider-container > * {                                                                                                                                                                          
        grid-row: 1;
        grid-column: 1;
     }

    .slider-labels {
      display: flex;
      flex-direction: column;
      justify-content: space-between;
      height: var(--surface-height,300px);
      font-size: 12px;
      user-select: none;
      width: 100%;
      align-items: flex-start;
      position: relative;
    }


    .slider-track {
      position: relative;
      justify-self: center;
      width: 6px;
      height: var(--surface-height,300px);
      background: #ccc;
      border-radius: 3px;
      cursor: pointer;
   }

    .slider-thumb {
      position: absolute;
      left: 50%;
      transform: translate(-50%, -50%);
      width: 25px;
      height: 50px;
      background: url("components/thumb.svg") no-repeat center center;
      background-size: 100% 100%;
      cursor: grab;
      touch-action: none;
    }

    .slider-thumb:active {
      cursor: grabbing;
    }
  `;

  static markup = `
    <!-- Component inner structure -->
    <div class="component">
       <div class="component-value"></div>
       <div class="slider-container">
          <div class="slider-labels"></div>
          <div class="slider-track">
            <div class="slider-thumb"></div>
          </div>
       </div>
       <div class="component-name"></div>
    </div>
  `;

  constructor() {
    super();
    this.params.min = 0;
    this.params.max = 100;
    this.params.integer = false;
  }

  connectedCallback()  {
     const name = getNameValue(this);
     if (name != null) this.params.name = name;
      let attr;
      if (attr = this.getAttribute("max")) {this.params.max = attr;}
      if (attr = this.getAttribute("min")) {this.params.min = attr;}
      if (attr = this.getAttribute("value")) {this.params.value = attr;}
      if (this.getAttribute("integer") == null) {this.params.integer = false;}
      if (this.getAttribute("integer") == "true") {this.params.integer = true;}
      this.shadowRoot.appendChild(this.createComponent(this.params));

  }

  createComponent(params) {
      const usePoints = Array.isArray(params.points) && params.points.length > 0;
      if (usePoints) {
        params.min = 0;
        params.max = params.points.length - 1;
        params.value = Math.max(params.min, Math.min(params.max, Math.round(params.value)));
      }

      // Lookup elements in the shadow tree (Defined in the template)
      const component = this.shadowRoot.querySelector(".component");
      const labelContainer = this.shadowRoot.querySelector(".slider-labels");
      const valueDisplay = this.shadowRoot.querySelector(".component-value");
      const nameDisplay = this.shadowRoot.querySelector(".component-name");
      const track = this.shadowRoot.querySelector(".slider-track");
      const thumb = this.shadowRoot.querySelector(".slider-thumb");

      if (usePoints) {
        for (let i = params.max; i >= params.min; i--) {
          const label = document.createElement("div");
          //label.className = "slider-label-text";
          label.textContent = params.points[i].label;
          labelContainer.appendChild(label);
        }
      }

      nameDisplay.textContent = params.name;

      const sliderHeight = track.offsetHeight;
      const steps = params.max - params.min;
      const stepHeight = sliderHeight / steps;
      this.stepheight = stepHeight;
      let dragging = false;
      let trackRect = null;

      // Convert value to top position, but invert so max = top=0, min=bottom=sliderHeight
      function valueToTop(val) {
        return (params.max - val) * stepHeight;
      }

      // Convert y position to value (inverted)
      function topToValue(top) {
        const val = params.max - top / stepHeight;
        return val;
      }

      function setValue(val) {
        let sendValue = false;
        val == null ? val =params.min : sendValue = true;
        val = Math.max(params.min, Math.min(params.max, usePoints ? Math.round(val) : val));
        const top = valueToTop(val);
        thumb.style.top = `${top}px`;
        if (params.integer) val = Math.round(val);
        valueDisplay.textContent = usePoints ? params.points[val].label : (Number.isInteger(val) ? val.toString() : val.toFixed(2));
        let state;
        if (usePoints) {
             state = { name: params.name, pos: val, value: params.points[val].value, label: params.points[val].label};
        } else {
             state = { name: params.name, value: val};
        }
        if (sendValue) {
            component.dispatchEvent(new CustomEvent('component-changed', {
              bubbles: true,
              composed: true,
              detail: state
            }));
        }
      }

      function updateFromPosition(clientY) {
        const offsetY = clientY - trackRect.top;
        const clampedY = Math.max(0, Math.min(sliderHeight, offsetY));
        let val = topToValue(clampedY);
        if (usePoints) val = Math.round(val);
        setValue(val);
      }

      function onPointerDown(e) {
        dragging = true;
        trackRect = track.getBoundingClientRect();
        const clientY = e.touches ? e.touches[0].clientY : e.clientY;
        updateFromPosition(clientY);
        document.addEventListener("mousemove", onPointerMove);
        document.addEventListener("mouseup", onPointerUp);
        document.addEventListener("touchmove", onPointerMove, { passive: false });
        document.addEventListener("touchend", onPointerUp);
      }

      function onPointerMove(e) {
        if (!dragging) return;
        const clientY = e.touches ? e.touches[0].clientY : e.clientY;
        updateFromPosition(clientY);
        e.preventDefault();
      }

      function onPointerUp() {
        dragging = false;
        document.removeEventListener("mousemove", onPointerMove);
        document.removeEventListener("mouseup", onPointerUp);
        document.removeEventListener("touchmove", onPointerMove);
        document.removeEventListener("touchend", onPointerUp);
      }

      thumb.addEventListener("mousedown", onPointerDown);
      thumb.addEventListener("touchstart", onPointerDown, { passive: false });

      track.addEventListener("click", (e) => {
        e.stopImmediatePropagation();
        trackRect = track.getBoundingClientRect();
        updateFromPosition(e.clientY);
      });

      setValue(params.value);
      return component;
    }

      setBus(bus) {
        if (this.bus == bus) return;
        this.bus = bus;

        this.bus.addEventListener('data-ready', e => {
          //console.log('catched data-ready',e);
          e.stopImmediatePropagation();
          if (this.params.value == null) {
            this.update(e.detail.value);
          }
        });
      }

   update(value) {
      const usePoints = Array.isArray(this.params.points) && this.params.points.length > 0;
      const thumb = this.shadowRoot.querySelector(".slider-thumb");
      const valueDisplay = this.shadowRoot.querySelector(".component-value");
      const component = this.shadowRoot.querySelector(".component");

      let val = Math.max(this.params.min, Math.min(this.params.max, usePoints ? Math.round(value) : value));
      const top = (this.params.max - val) * this.stepheight;
      thumb.style.top = `${top}px`;
      if (this.params.integer) val = Math.round(val);
      valueDisplay.textContent = usePoints ? this.params.points[val].label : (Number.isInteger(val) ? val.toString() : val.toFixed(2));
      let state;
      if (usePoints) {
             state = { name: this.params.name, pos: val, value: this.params.points[val].value, label: this.params.points[val].label};
      } else {
             state = { name: this.params.name, value: val};
      }
            component.dispatchEvent(new CustomEvent('component-changed', {
              bubbles: true,
              composed: true,
              detail: state
            }));
    }

}

customElements.define('slider-view', SliderView);

