const API_BASE = 'http://localhost:8080';

function apiPost(path, data)
{
  return fetch(`${API_BASE}${path}`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(data)
  })
  .then(r => {
    console.log(`POST ${path}`, r.status, r.statusText);
    if (!r.ok) throw new Error(`HTTP ${r.status}`);
    return r.json();
  });
}
