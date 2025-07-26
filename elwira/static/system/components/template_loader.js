// template_loader.js
const templateCache = new Map();

export async function loadTemplate(url) {
  if (templateCache.has(url)) {
    return templateCache.get(url);
  }

  const response = await fetch(url);
  const text = await response.text();

  const template = document.createElement('template');
  template.innerHTML = text;

  templateCache.set(url, template);
  return template;
}
