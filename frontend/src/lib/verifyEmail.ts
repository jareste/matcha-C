import { apiPost } from './api.ts'

// Outcome of POST /api/verify-email, as shown by the verification page.
// 'error' covers everything that is not about the token itself (network, 5xx):
// the link may still be good, so the user can retry.
export type VerifyOutcome =
  | { kind: 'success' }
  | { kind: 'expired' }
  | { kind: 'invalid' }
  | { kind: 'error'; message: string }

// Maps the backend answer to an outcome. The status is checked first; the
// message is a fallback in case the backend answers 400 for every token error.
export function verifyOutcome(res: { ok: boolean; status: number; error?: string }): VerifyOutcome {
  if (res.ok) return { kind: 'success' }
  const message = res.error ?? ''
  if (res.status === 410 || /expired/i.test(message)) return { kind: 'expired' }
  if (res.status === 400 || res.status === 404) return { kind: 'invalid' }
  return { kind: 'error', message: message || `Request failed (${res.status})` }
}

export async function verifyEmail(token: string): Promise<VerifyOutcome> {
  return verifyOutcome(await apiPost('/api/verify-email', { token }))
}
