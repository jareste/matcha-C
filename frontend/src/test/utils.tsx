import { render } from '@testing-library/react'
import { MemoryRouter } from 'react-router'
import { vi, type Mock } from 'vitest'
import App from '../App.tsx'

// Renders the whole app (routes + layout) at the given URL.
export function renderAt(path: string) {
  return render(
    <MemoryRouter initialEntries={[path]}>
      <App />
    </MemoryRouter>,
  )
}

export function jsonResponse(status: number, body: unknown) {
  return new Response(JSON.stringify(body), { status, headers: { 'Content-Type': 'application/json' } })
}

// Fake backend session, read by GET /api/validate. null = no cookie (403).
export const session: { username: string | null } = { username: null }

// Stubs fetch: GET /api/validate answers from `session`, every other call goes
// to `api`, so tests only see (and assert on) the calls they care about.
export function stubApi(api: Mock = vi.fn()): Mock {
  vi.stubGlobal('fetch', (input: RequestInfo | URL, init?: RequestInit) => {
    if (String(input).endsWith('/api/validate')) {
      return Promise.resolve(
        session.username
          ? jsonResponse(200, { success: true, message: 'User is valid', username: session.username })
          : jsonResponse(403, { error: 'Forbidden' }),
      )
    }
    return api(input, init)
  })
  return api
}
