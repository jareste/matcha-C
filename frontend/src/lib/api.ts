const API_BASE: string = import.meta.env.VITE_API_URL ?? 'http://localhost:8080'

export type ApiResult<T> =
  | { ok: true; status: number; data: T }
  | { ok: false; status: number; error: string }

// The backend always answers errors as {"error": "..."} (see api.md).
// status 0 means the request never reached the server.
export async function apiPost<T>(path: string, body: unknown): Promise<ApiResult<T>> {
  let res: Response
  try {
    res = await fetch(`${API_BASE}${path}`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      credentials: 'include',
      body: JSON.stringify(body),
    })
  } catch {
    return { ok: false, status: 0, error: 'Cannot reach the server. Please try again.' }
  }

  const data = await res.json().catch(() => null)
  if (res.ok) return { ok: true, status: res.status, data: data as T }
  const error = typeof data?.error === 'string' ? data.error : `Request failed (${res.status})`
  return { ok: false, status: res.status, error }
}
