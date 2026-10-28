import { useCallback, useEffect, useMemo, useState, type ReactNode } from 'react'
import { apiGet, apiPost } from '../lib/api.ts'
import { AuthContext, type AuthContextValue, type AuthStatus, type User } from '../lib/authContext.ts'

type ValidateResponse = { success: boolean; username: string }

// Asks the backend who owns the current cookie. No cookie answers 403 and an
// invalid one drops the connection (status 0): both mean "not logged in".
async function fetchSession(): Promise<User | null> {
  const res = await apiGet<ValidateResponse>('/api/validate')
  return res.ok && res.data?.username ? { username: res.data.username } : null
}

function AuthProvider({ children }: { children: ReactNode }) {
  const [user, setUser] = useState<User | null>(null)
  const [status, setStatus] = useState<AuthStatus>('loading')

  const applySession = useCallback((next: User | null) => {
    setUser(next)
    setStatus(next ? 'authenticated' : 'anonymous')
  }, [])

  useEffect(() => {
    let cancelled = false
    void fetchSession().then((next) => {
      if (!cancelled) applySession(next)
    })
    return () => {
      cancelled = true
    }
  }, [applySession])

  const login = useCallback<AuthContextValue['login']>(
    async (email, password) => {
      const res = await apiPost('/api/login', { email, password })
      if (!res.ok) return { ok: false, error: res.error }

      const next = await fetchSession()
      if (!next) return { ok: false, error: 'Your session could not be started. Please try again.' }
      applySession(next)
      return { ok: true }
    },
    [applySession],
  )

  const logout = useCallback<AuthContextValue['logout']>(async () => {
    const res = await apiPost('/api/logout')
    // 403 means the cookie is already gone: the user is logged out either way.
    if (!res.ok && res.status !== 403) return { ok: false, error: res.error }
    applySession(null)
    return { ok: true }
  }, [applySession])

  const value = useMemo(() => ({ status, user, login, logout }), [status, user, login, logout])
  return <AuthContext.Provider value={value}>{children}</AuthContext.Provider>
}

export default AuthProvider
