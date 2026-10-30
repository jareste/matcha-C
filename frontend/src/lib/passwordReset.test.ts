import { afterEach, describe, expect, test, vi } from 'vitest'
import { forgotOutcome, newPasswordError, requestPasswordReset, resetOutcome, resetPassword } from './passwordReset.ts'

function jsonResponse(status: number, body: unknown) {
  return new Response(JSON.stringify(body), { status, headers: { 'Content-Type': 'application/json' } })
}

afterEach(() => {
  vi.unstubAllGlobals()
})

describe('forgotOutcome', () => {
  test('sent on a 2xx answer', () => {
    expect(forgotOutcome({ ok: true, status: 200 })).toEqual({ kind: 'sent' })
  })

  test('still "sent" when the backend reveals the email is unknown', () => {
    expect(forgotOutcome({ ok: false, status: 400, error: 'Email not found' })).toEqual({ kind: 'sent' })
    expect(forgotOutcome({ ok: false, status: 404, error: 'User not found' })).toEqual({ kind: 'sent' })
  })

  test('error on malformed request, server or network failure', () => {
    expect(forgotOutcome({ ok: false, status: 400, error: 'Email is required' })).toEqual({
      kind: 'error',
      message: 'Email is required',
    })
    expect(forgotOutcome({ ok: false, status: 500, error: 'Failed to send email' })).toEqual({
      kind: 'error',
      message: 'Failed to send email',
    })
    expect(forgotOutcome({ ok: false, status: 0, error: 'Cannot reach the server.' })).toEqual({
      kind: 'error',
      message: 'Cannot reach the server.',
    })
  })
})

describe('requestPasswordReset', () => {
  test('posts the trimmed email to /api/forgot-password', async () => {
    const fetchMock = vi.fn().mockResolvedValue(jsonResponse(200, { success: true }))
    vi.stubGlobal('fetch', fetchMock)

    expect(await requestPasswordReset('  a@b.co ')).toEqual({ kind: 'sent' })
    const [url, init] = fetchMock.mock.calls[0]
    expect(url).toMatch(/\/api\/forgot-password$/)
    expect(JSON.parse(init.body)).toEqual({ email: 'a@b.co' })
  })
})

describe('resetOutcome', () => {
  test('success on a 2xx answer', () => {
    expect(resetOutcome({ ok: true, status: 200 })).toEqual({ kind: 'success' })
  })

  test('expired on 410 or when the message says so', () => {
    expect(resetOutcome({ ok: false, status: 410, error: 'Token expired' })).toEqual({ kind: 'expired' })
    expect(resetOutcome({ ok: false, status: 400, error: 'Token expired' })).toEqual({ kind: 'expired' })
  })

  test('weak when the backend rejects the password itself', () => {
    expect(resetOutcome({ ok: false, status: 400, error: 'Password too weak' })).toEqual({
      kind: 'weak',
      message: 'Password too weak',
    })
  })

  test('invalid on 400 and 404 token errors', () => {
    expect(resetOutcome({ ok: false, status: 400, error: 'Invalid token' })).toEqual({ kind: 'invalid' })
    expect(resetOutcome({ ok: false, status: 404, error: 'Not Found' })).toEqual({ kind: 'invalid' })
  })

  test('error on server or network failure', () => {
    expect(resetOutcome({ ok: false, status: 500, error: 'Failed to reset password' })).toEqual({
      kind: 'error',
      message: 'Failed to reset password',
    })
  })
})

describe('resetPassword', () => {
  test('posts the token and the new password to /api/reset-password', async () => {
    const fetchMock = vi.fn().mockResolvedValue(jsonResponse(200, { success: true }))
    vi.stubGlobal('fetch', fetchMock)

    expect(await resetPassword('tok', 'Strong1pass')).toEqual({ kind: 'success' })
    const [url, init] = fetchMock.mock.calls[0]
    expect(url).toMatch(/\/api\/reset-password$/)
    expect(JSON.parse(init.body)).toEqual({ token: 'tok', password: 'Strong1pass' })
  })
})

describe('newPasswordError', () => {
  test('accepts a strong, confirmed password', () => {
    expect(newPasswordError('Strong1pass', 'Strong1pass')).toEqual({})
  })

  test('rejects a weak password with the failed rules', () => {
    expect(newPasswordError('short', 'short').password).toMatch(/at least 8 characters/)
  })

  test('requires a matching confirmation', () => {
    expect(newPasswordError('Strong1pass', '').confirm).toBe('Please confirm your password')
    expect(newPasswordError('Strong1pass', 'Other1pass').confirm).toBe('Passwords do not match')
  })
})
