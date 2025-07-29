import { ViewComponent } from './view_component.js';

/*
TODO: make current available semantics selectable via attributes
      - snapping to label positions
      - float or integer
      - show label or value in value field
      - setting attributes to  snapping and or showing labels in value field should produce error if no points (<option>:s) defined
*/


export class ElviraSlider extends ViewComponent {
  params = {
      prio: 0,
      max: 100,
      min: 0,
      integer: true,
      name: 'anonymous',
      value: 37,
      points: [],
  }
  constructor() {
    super('./components/elvira-slider.html');
  }

  onStart() {
    //console.log('Params', this.params);
    this.shadowRoot.appendChild(this.createComponent(this.params));
  }


  createComponent(params) {
      const usePoints = Array.isArray(params.points) && params.points.length > 0;
      
      if (usePoints) {
        params.min = 0;
        params.max = params.points.length - 1;
        params.value = Math.max(params.min, Math.min(params.max, Math.round(params.value)));
      }
      //let slider_state = {};

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

      nameDisplay.textContent = this.params.name;

      const sliderHeight = track.offsetHeight;
      //const sliderHeight = 300;
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

  Promise.resolve().then(() => {
    requestAnimationFrame(() => {
        component.dispatchEvent(new CustomEvent("view-change", {detail: state, bubbles: true, composed: true} ));
    });
  });

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
        e.stopPropagation();
        trackRect = track.getBoundingClientRect();
        updateFromPosition(e.clientY);
      });

      setValue(params.value);
      return component;
    }

    update(info) {
      console.warn('elvira-slider update',info);
      const usePoints = Array.isArray(this.params.points) && this.params.points.length > 0;
      const thumb = this.shadowRoot.querySelector(".slider-thumb");
      const valueDisplay = this.shadowRoot.querySelector(".component-value");
      const component = this.shadowRoot.querySelector(".component");


        let val = Math.max(this.params.min, Math.min(this.params.max, usePoints ? Math.round(info.value) : info.value));
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
        component.dispatchEvent(new CustomEvent("view-change", {detail: state, bubbles: true, composed: true} ));

    }
}

customElements.define('elvira-slider', ElviraSlider);

