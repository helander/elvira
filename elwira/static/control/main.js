
async function buildUI() {
  const params = new URLSearchParams(window.location.search);
  const context = params.get('context');
  if (context == null) {
    alert('NO context defined');
    return;
  }

  const container = document.getElementById('dynamic-container');
  const response = await fetch('/pairs/'+context);
  const configs = await response.json();

  // Sort by descending prio
  configs.sort((a, b) => b.prio - a.prio);

  for (const { endpoint, view, name } of configs) {
    const pair = document.createElement('device-control');
    const endpoint_el = document.createElement(endpoint.element);
    Object.keys(endpoint).forEach(key => {
       if (key != 'element') {
         endpoint_el.setAttribute(key,endpoint[key]);
       }
    });

    const view_el = document.createElement(view.element);
    Object.keys(view).forEach(key => {
       if (key != 'element') {
         if (key == 'points') {
           const points = view[key];
           view_el.params.points = points;
         } else {
           view_el.setAttribute(key,view[key]);
         }
       }
    });

    pair.setAttribute("name",name);
    pair.appendChild(endpoint_el);
    pair.appendChild(view_el);
    container.appendChild(pair);
  }

}

buildUI();
