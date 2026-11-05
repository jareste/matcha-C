import { useCallback, useEffect, useMemo, useRef, useState, type ReactNode } from 'react'
import { apiGet, apiPost, onSessionLost } from '../lib/api.ts'
import { AuthContext, type AuthContextValue, type AuthStatus, type User } from '../lib/authContext.ts'

type ValidateResponse = { success: boolean; username: string }

const AUTH_CALL = { ignoreSessionLoss: true }

// Asks the backend who owns the current cookie. No cookie (403), an invalid or
// expired one (401) and an unreachable server (status 0) all mean "not logged in".
async function fetchSession(): Promise<User | null> {
  const res = await apiGet<ValidateResponse>('/api/validate', AUTH_CALL)
  return res.ok && res.data?.username ? { username: res.data.username } : null
}

function AuthProvider({ children }: { children: ReactNode }) {
  const [user, setUser] = useState<User | null>(null)
  const [status, setStatus] = useState<AuthStatus>('loading')
  const [sessionExpired, setSessionExpired] = useState(false)
  // Read synchronously by the session-lost handler, so that several 401s
  // arriving together clear the state (and redirect) only once.
  const statusRef = useRef<AuthStatus>('loading')

  const applySession = useCallback((next: User | null) => {
    statusRef.current = next ? 'authenticated' : 'anonymous'
    setUser(next)
    setStatus(statusRef.current)
    if (next) setSessionExpired(false)
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

  // Any API call answering 401 while logged in ends the session locally.
  // <RequireAuth> then sends the user to /login; nothing here navigates, so a
  // public page that gets a 401 cannot start a redirect loop.
  useEffect(
    () =>
      onSessionLost(() => {
        if (statusRef.current !== 'authenticated') return
        applySession(null)
        setSessionExpired(true)
      }),
    [applySession],
  )

  const login = useCallback<AuthContextValue['login']>(
    async (email, password) => {
      const res = await apiPost('/api/login', { email, password }, AUTH_CALL)
      if (!res.ok) return { ok: false, error: res.error }

      const next = await fetchSession()
      if (!next) return { ok: false, error: 'Your session could not be started. Please try again.' }
      applySession(next)
      return { ok: true }
    },
    [applySession],
  )

  const logout = useCallback<AuthContextValue['logout']>(async () => {
    const res = await apiPost('/api/logout', undefined, AUTH_CALL)
    // 401/403 mean the cookie is already gone: the user is logged out either way.
    if (!res.ok && res.status !== 401 && res.status !== 403) return { ok: false, error: res.error }
    applySession(null)
    return { ok: true }
  }, [applySession])

  const value = useMemo(
    () => ({ status, user, sessionExpired, login, logout }),
    [status, user, sessionExpired, login, logout],
  )
  return <AuthContext.Provider value={value}>{children}</AuthContext.Provider>
}

export default AuthProvider
