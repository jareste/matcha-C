const API_BASE = 'http://localhost:8080';

function apiPost(path, data)
{
  return fetch(`${API_BASE}${path}`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    credentials: 'include',
    body: JSON.stringify(data)
  })
  .then(r => {
    if (!r.ok) 
    {
      return r.json();
    }
    return r.json();
  })
  .catch(err => {
    return null;
  });
}

function apiGet(path)
{
  return fetch(`${API_BASE}${path}`, {
    method: 'GET',
    credentials: 'include'
  })
  .then(r => {
    if (!r.ok) 
    {
      return r.json();
    }
    return r.json();
  });
}
