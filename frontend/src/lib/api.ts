const API_BASE: string = import.meta.env.VITE_API_URL ?? 'http://localhost:8080'

export type ApiResult<T> =
  | { ok: true; status: number; data: T }
  | { ok: false; status: number; error: string }

export type RequestOptions = {
  // Set on the auth calls themselves (session probe, login, logout): their
  // "no session" answers are expected and must not count as a lost session.
  ignoreSessionLoss?: boolean
}

// A request answered "you have no valid session". 401 is the agreed answer
// (see api.md); 403 "Forbidden" is what the auth middleware sends today when
// the cookie is missing, which is how an expired cookie looks to the server.
// Status 0 is left out: it is also a plain network failure.
export function isSessionLost(status: number, error: string): boolean {
  return status === 401 || (status === 403 && error === 'Forbidden')
}

let sessionLostHandler: (() => void) | null = null

// Registered once by <AuthProvider>. Returns the cleanup for its effect.
export function onSessionLost(handler: () => void): () => void {
  sessionLostHandler = handler
  return () => {
    if (sessionLostHandler === handler) sessionLostHandler = null
  }
}

// The backend always answers errors as {"error": "..."} (see api.md).
// status 0 means no usable answer: the server is unreachable or dropped the connection.
async function apiRequest<T>(
  method: 'GET' | 'POST',
  path: string,
  body: unknown,
  options: RequestOptions,
): Promise<ApiResult<T>> {
  let res: Response
  try {
    res = await fetch(`${API_BASE}${path}`, {
      method,
      credentials: 'include',
      ...(body !== undefined && {
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(body),
      }),
    })
  } catch {
    return { ok: false, status: 0, error: 'Cannot reach the server. Please try again.' }
  }

  const data = await res.json().catch(() => null)
  if (res.ok) return { ok: true, status: res.status, data: data as T }
  const error = typeof data?.error === 'string' ? data.error : `Request failed (${res.status})`
  if (!options.ignoreSessionLoss && isSessionLost(res.status, error)) sessionLostHandler?.()
  return { ok: false, status: res.status, error }
}

export function apiGet<T>(path: string, options: RequestOptions = {}): Promise<ApiResult<T>> {
  return apiRequest<T>('GET', path, undefined, options)
}

export function apiPost<T>(path: string, body?: unknown, options: RequestOptions = {}): Promise<ApiResult<T>> {
  return apiRequest<T>('POST', path, body, options)
}
