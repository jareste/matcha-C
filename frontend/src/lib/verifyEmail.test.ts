import { afterEach, describe, expect, test, vi } from 'vitest'
import { verifyEmail, verifyOutcome } from './verifyEmail.ts'

function jsonResponse(status: number, body: unknown) {
  return new Response(JSON.stringify(body), { status, headers: { 'Content-Type': 'application/json' } })
}

afterEach(() => {
  vi.unstubAllGlobals()
})

describe('verifyOutcome', () => {
  test('success on a 2xx answer', () => {
    expect(verifyOutcome({ ok: true, status: 200 })).toEqual({ kind: 'success' })
  })

  test('expired on 410', () => {
    expect(verifyOutcome({ ok: false, status: 410, error: 'Gone' })).toEqual({ kind: 'expired' })
  })

  test('expired when a 400 message says so', () => {
    expect(verifyOutcome({ ok: false, status: 400, error: 'Token expired' })).toEqual({ kind: 'expired' })
  })

  test('invalid on 400 and 404', () => {
    expect(verifyOutcome({ ok: false, status: 400, error: 'Invalid token' })).toEqual({ kind: 'invalid' })
    expect(verifyOutcome({ ok: false, status: 404, error: 'Not Found' })).toEqual({ kind: 'invalid' })
  })

  test('error with the backend message on server or network failure', () => {
    expect(verifyOutcome({ ok: false, status: 500, error: 'Failed to verify email' })).toEqual({
      kind: 'error',
      message: 'Failed to verify email',
    })
    expect(verifyOutcome({ ok: false, status: 0, error: 'Cannot reach the server.' })).toEqual({
      kind: 'error',
      message: 'Cannot reach the server.',
    })
  })
})

describe('verifyEmail', () => {
  test('posts the token to /api/verify-email', async () => {
    const fetchMock = vi.fn().mockResolvedValue(jsonResponse(200, { success: true }))
    vi.stubGlobal('fetch', fetchMock)

    expect(await verifyEmail('abc123')).toEqual({ kind: 'success' })
    const [url, init] = fetchMock.mock.calls[0]
    expect(url).toMatch(/\/api\/verify-email$/)
    expect(JSON.parse(init.body)).toEqual({ token: 'abc123' })
  })

  test('reports the backend rejection', async () => {
    vi.stubGlobal('fetch', vi.fn().mockResolvedValue(jsonResponse(410, { error: 'Token expired' })))
    expect(await verifyEmail('old')).toEqual({ kind: 'expired' })
  })
})
