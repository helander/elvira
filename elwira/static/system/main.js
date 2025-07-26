//import './components/data-pair.js';
//import './components/data-source-a.js';
//import './components/view-table.js';
//import './components/view-chart.js';

const sourceTypeToTag = {
  c: 'data-source-c',
  b: 'data-source-b',
  a: 'data-source-a'
};

async function buildUI() {
  const container = document.getElementById('app');
  const response = await fetch('./ui-config.json');
  const configs = await response.json();

  // Sort by descending prio
  configs.sort((a, b) => b.prio - a.prio);

  for (const { type, endpoint, view, name } of configs) {
    const pair = document.createElement('data-pair');
    console.log('pair',pair);

    const sourceTag = sourceTypeToTag[type];
    console.log('source',sourceTag);
    const source = document.createElement(sourceTag);
    //source.setAttribute('endpoint', endpoint);

    const viewEl = document.createElement(view);
    console.log('view',view);
    if (typeof viewEl.setName === 'function') {
      viewEl.setName(name);
    } else {
      viewEl.name = name;
    }

    pair.appendChild(source);
    pair.appendChild(viewEl);
    container.appendChild(pair);
  }
}

buildUI();
