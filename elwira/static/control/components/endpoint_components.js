function getContextValue(start) {
  let el = start;
  while (el) {
    if (el.hasAttribute?.('context')) {
      return el.getAttribute('context');
    }
    el = el.parentNode || el.host;  // climb up light + shadow DOM
  }

  const params = new URLSearchParams(window.location.search);
  return params.get('context');
}



///////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Element tag: <lv2control-endpoint>
//   Element class: LV2ControlEndpoint
//
///////////////////////////////////////////////////////////////////////////////////////////////////////

export class LV2ControlEndpoint extends HTMLElement {
  constructor() {
    super();
  }

  connectedCallback() {
    this.context = getContextValue(this);
    this.classList.add('endpoint');
    this.index = this.getAttribute("index");
    if (this.index == null) {
       console.error('No index defined on element',this);
    }
  }

  setBus(bus) {
        if (this.bus == bus) return;
        this.bus = bus;

        this.url = this.getAttribute('url');

        this.bus.addEventListener('view-changed', async e => {
          //console.log('catched view-changed',e);
          try {
            if (this.index != null) {
                const serviceUrl = "/metadata/"+this.context+"?key=control.in."+this.index+"&value="+e.detail.value;
                const response = await fetch(serviceUrl);
            }
          } catch (err) {
            console.error(`[endpoint] PATCH failed:`, err);
          }
        });

        setTimeout(() => this.fetchInitialValue(), 0);
  }

  async fetchInitialValue() {
        if (this.index == null) return;
        const serviceUrl = "/node-props/"+this.context+"?prefix=elvira.control.in";
        try {
               const response = await fetch(serviceUrl);
               const data = await response.json();
               let current = null;
               if (data != null && data.properties != null) {
                   current = data.properties["elvira.control.in."+this.index];
               }
               if (current != null) {
                  this.bus.dispatchEvent(new CustomEvent('data-ready', {
                     detail: {value: current},
                     bubbles: false,
                     composed: false
                  }));
               }

        } catch (err) {
          console.error(`[endpoint] GET failed from ${serviceUrl}:`, err);
        }
  }


}

customElements.define('lv2control-endpoint', LV2ControlEndpoint);


///////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Element tag: <midicc-endpoint>
//   Element class: MidiccEndpoint
//
///////////////////////////////////////////////////////////////////////////////////////////////////////


export class MidiccEndpoint extends HTMLElement {
  constructor() {
    super();
  }

  connectedCallback() {
    this.context = getContextValue(this);
    this.index = this.getAttribute("index");
    if (this.index == null) {
       console.error('No index defined on element',this);
    }
    this.classList.add('endpoint');
  }

  setBus(bus) {
        if (this.bus == bus) return;
        this.bus = bus;

        this.url = this.getAttribute('url');

        this.bus.addEventListener('view-changed', async e => {
          //console.log('catched view-changed',e);
          try {
            if (this.index != null) {
                const serviceUrl = "/metadata/"+this.context+"?key=midicc."+this.index+"&value="+e.detail.value;
                const response = await fetch(serviceUrl);
            }
          } catch (err) {
            console.error(`[endpoint] PATCH failed:`, err);
          }
        });

        setTimeout(() => this.fetchInitialValue(), 0);
  }

  async fetchInitialValue() {
        if (this.index == null) return;
        const serviceUrl = "/node-props/"+this.context+"?prefix=elvira.midicc";
        try {
               const response = await fetch(serviceUrl);
               const data = await response.json();
               let current = null;
               if (data != null && data.properties != null) {
                   current = data.properties["elvira.midicc."+this.index];
               }
               if (current != null) {
                  this.bus.dispatchEvent(new CustomEvent('data-ready', {
                     detail: {value: current},
                     bubbles: false,
                     composed: false
                  }));
               }

        } catch (err) {
          console.error(`[endpoint] GET failed from ${serviceUrl}:`, err);
        }
  }

}

customElements.define('midicc-endpoint', MidiccEndpoint);


///////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Element tag: <lv2parameter-endpoint>
//   Element class: LV2ParameterEndpoint
//
///////////////////////////////////////////////////////////////////////////////////////////////////////

export class LV2ParameterEndpoint extends HTMLElement {
  constructor() {
    super();
  }

  connectedCallback() {
    this.context = getContextValue(this);
    this.classList.add('endpoint');
    this.uri = this.getAttribute("uri");
    if (this.uri == null) {
       console.error('No uri defined on element',this);
    }
  }

  setBus(bus) {
        if (this.bus == bus) return;
        this.bus = bus;

        this.url = this.getAttribute('url');

        this.bus.addEventListener('view-changed', async e => {
          //console.log('catched view-changed',e);
          try {
            if (this.uri != null) {
                const serviceUrl = "/metadata/"+this.context+"?key="+encodeURIComponent(this.uri)+"&value="+encodeURIComponent(e.detail.value);
                const response = await fetch(serviceUrl);
            }
          } catch (err) {
            console.error(`[endpoint] PATCH failed:`, err);
          }
        });

  }

}

customElements.define('lv2parameter-endpoint', LV2ParameterEndpoint);
