import { apiPost } from './api.ts'
import { failedPasswordRules } from './register.ts'

// Outcome of POST /api/forgot-password, as shown by the request page.
// The page must never reveal whether the email is registered, so "unknown
// email" answers are folded into 'sent'. Only failures that are clearly not
// about the account (network, 5xx, malformed request) surface as 'error'.
export type ForgotOutcome = { kind: 'sent' } | { kind: 'error'; message: string }

const ACCOUNT_HINT_RE = /not found|unknown|no account|does not exist|not registered/i

export function forgotOutcome(res: { ok: boolean; status: number; error?: string }): ForgotOutcome {
  if (res.ok) return { kind: 'sent' }
  const message = res.error ?? ''
  if (res.status === 404 || (res.status === 400 && ACCOUNT_HINT_RE.test(message))) return { kind: 'sent' }
  return { kind: 'error', message: message || `Request failed (${res.status})` }
}

export async function requestPasswordReset(email: string): Promise<ForgotOutcome> {
  return forgotOutcome(await apiPost('/api/forgot-password', { email: email.trim() }))
}

// Outcome of POST /api/reset-password, as shown by the reset page.
// Same shape as the email verification flow: the token is checked first,
// anything else is a retryable error.
export type ResetOutcome =
  | { kind: 'success' }
  | { kind: 'expired' }
  | { kind: 'invalid' }
  | { kind: 'weak'; message: string }
  | { kind: 'error'; message: string }

export function resetOutcome(res: { ok: boolean; status: number; error?: string }): ResetOutcome {
  if (res.ok) return { kind: 'success' }
  const message = res.error ?? ''
  if (res.status === 410 || /expired/i.test(message)) return { kind: 'expired' }
  if (res.status === 400 && /password/i.test(message)) return { kind: 'weak', message }
  if (res.status === 400 || res.status === 404) return { kind: 'invalid' }
  return { kind: 'error', message: message || `Request failed (${res.status})` }
}

export async function resetPassword(token: string, password: string): Promise<ResetOutcome> {
  return resetOutcome(await apiPost('/api/reset-password', { token, password }))
}

// Client-side check of the new password, same rules as the register form.
// Returns an empty string when the password is acceptable.
export function newPasswordError(password: string, confirm: string): { password?: string; confirm?: string } {
  const errors: { password?: string; confirm?: string } = {}
  if (!password) errors.password = 'Password is required'
  else {
    const failed = failedPasswordRules(password)
    if (failed.length) errors.password = `Password needs: ${failed.map((r) => r.label.toLowerCase()).join(', ')}`
  }
  if (!confirm) errors.confirm = 'Please confirm your password'
  else if (confirm !== password) errors.confirm = 'Passwords do not match'
  return errors
}
