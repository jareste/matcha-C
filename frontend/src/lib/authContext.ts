import { createContext, useContext } from 'react'

// The session lives in the HttpOnly `token` cookie set by POST /api/login (see api.md).
// The frontend never sees it and never stores anything about it: the only state
// kept is the username returned by GET /api/validate, held in memory by <AuthProvider>.

export type User = { username: string }

export type AuthStatus = 'loading' | 'authenticated' | 'anonymous'

export type AuthResult = { ok: true } | { ok: false; error: string }

export type AuthContextValue = {
  status: AuthStatus
  user: User | null
  login: (email: string, password: string) => Promise<AuthResult>
  logout: () => Promise<AuthResult>
}

export const AuthContext = createContext<AuthContextValue | null>(null)

export function useAuth(): AuthContextValue {
  const ctx = useContext(AuthContext)
  if (!ctx) throw new Error('useAuth must be used inside <AuthProvider>')
  return ctx
}
