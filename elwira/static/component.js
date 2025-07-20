
let cachedTemplate = null;

export class SliderComponent extends HTMLElement {
  constructor() {
    super();
    this.attachShadow({ mode: 'open' });
  }

  async connectedCallback() {
    if (!cachedTemplate) {
       const res = await fetch('./slider.html');
       const html = await res.text();
       const tplDoc = document.createElement('div');
       tplDoc.innerHTML = html.trim();
       cachedTemplate = tplDoc.querySelector('template');
    }
    if (!cachedTemplate) throw new Error('No <template> found in slider.html');

    this.shadowRoot.appendChild(cachedTemplate.content.cloneNode(true));
    this.shadowRoot.appendChild(this.createSlider(this.params));
  }


    createSlider(params) {
      const usePoints = Array.isArray(params.points) && params.points.length > 0;
      
      // If with labels, override min/max for internal logic
      if (usePoints) {
        params.min = 0;
        params.max = params.points.length - 1;
        params.value = Math.max(params.min, Math.min(params.max, Math.round(params.value)));
      }
      let slider_state = {};

      // Lookup elements in the shadow tree (Defined in the template)
      const wrapper = this.shadowRoot.querySelector(".slider-wrapper");
      const labelContainer = this.shadowRoot.querySelector(".slider-labels");
      const valueDisplay = this.shadowRoot.querySelector(".slider-value");
      const nameDisplay = this.shadowRoot.querySelector(".slider-name");
      const track = this.shadowRoot.querySelector(".slider-track");
      const thumb = this.shadowRoot.querySelector(".slider-thumb");

      if (usePoints) {
        for (let i = params.max; i >= params.min; i--) {
          const label = document.createElement("div");
          label.textContent = params.points[i].label;
          labelContainer.appendChild(label);
        }
      }

      nameDisplay.textContent = params.name;

      const sliderHeight = 300;
      const steps = params.max - params.min;
      const stepHeight = sliderHeight / steps;
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
        if (usePoints) {
             slider_state = { name: params.name, pos: val, value: params.points[val].value, label: params.points[val].label};
        } else {
             slider_state = { name: params.name, value: val};
        }
        wrapper.dispatchEvent(new CustomEvent("slider-change", {detail: slider_state, bubbles: true, composed: true} ));
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
      return wrapper;
    }
}

customElements.define('slider-component', SliderComponent);

