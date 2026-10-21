import { afterEach, describe, expect, test, vi } from 'vitest'
import { apiPost } from './api.ts'

function jsonResponse(status: number, body: unknown) {
  return new Response(JSON.stringify(body), { status, headers: { 'Content-Type': 'application/json' } })
}

afterEach(() => {
  vi.unstubAllGlobals()
})

describe('apiPost', () => {
  test('sends JSON with credentials and returns the data on success', async () => {
    const fetchMock = vi.fn().mockResolvedValue(jsonResponse(201, { success: true }))
    vi.stubGlobal('fetch', fetchMock)

    const res = await apiPost('/api/register', { a: 1 })

    expect(res).toEqual({ ok: true, status: 201, data: { success: true } })
    const [url, init] = fetchMock.mock.calls[0]
    expect(url).toMatch(/\/api\/register$/)
    expect(init).toMatchObject({
      method: 'POST',
      credentials: 'include',
      headers: { 'Content-Type': 'application/json' },
      body: '{"a":1}',
    })
  })

  test('returns the backend error message', async () => {
    vi.stubGlobal('fetch', vi.fn().mockResolvedValue(jsonResponse(400, { error: 'Email already exists' })))
    expect(await apiPost('/x', {})).toEqual({ ok: false, status: 400, error: 'Email already exists' })
  })

  test('falls back to a generic message when the body is not JSON', async () => {
    vi.stubGlobal('fetch', vi.fn().mockResolvedValue(new Response('oops', { status: 502 })))
    expect(await apiPost('/x', {})).toEqual({ ok: false, status: 502, error: 'Request failed (502)' })
  })

  test('reports a network failure with status 0', async () => {
    vi.stubGlobal('fetch', vi.fn().mockRejectedValue(new TypeError('Failed to fetch')))
    const res = await apiPost('/x', {})
    expect(res.ok).toBe(false)
    expect(res.status).toBe(0)
  })
})
