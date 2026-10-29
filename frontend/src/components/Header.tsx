import { useState } from 'react'
import { Link, NavLink, useNavigate } from 'react-router'
import { useAuth } from '../lib/authContext.ts'

const publicLinks = [
  { to: '/login', label: 'Log in' },
  { to: '/register', label: 'Sign up' },
]

const privateLinks = [{ to: '/', label: 'Home' }]

function navClass({ isActive }: { isActive: boolean }) {
  return [
    'block rounded-md px-3 py-2 text-sm font-medium transition-colors',
    isActive
      ? 'bg-brand/10 text-brand'
      : 'text-gray-600 hover:bg-gray-100 hover:text-gray-900 dark:text-gray-300 dark:hover:bg-gray-800 dark:hover:text-white',
  ].join(' ')
}

const logoutClass =
  'block w-full rounded-md px-3 py-2 text-left text-sm font-medium text-gray-600 transition-colors hover:bg-gray-100 hover:text-gray-900 disabled:opacity-60 dark:text-gray-300 dark:hover:bg-gray-800 dark:hover:text-white'

function Header() {
  const { status, logout } = useAuth()
  const navigate = useNavigate()
  const [open, setOpen] = useState(false)
  const [loggingOut, setLoggingOut] = useState(false)
  const [logoutError, setLogoutError] = useState('')

  const authenticated = status === 'authenticated'
  // While the session is being checked, show no auth-dependent links.
  const links = authenticated ? privateLinks : status === 'anonymous' ? publicLinks : []

  async function handleLogout() {
    setOpen(false)
    setLogoutError('')
    setLoggingOut(true)
    const res = await logout()
    setLoggingOut(false)
    if (res.ok) navigate('/login', { replace: true })
    else setLogoutError(`Could not log out: ${res.error}`)
  }

  function navItems(onPick?: () => void) {
    return (
      <>
        {links.map((l) => (
          <li key={l.to}>
            <NavLink to={l.to} end className={navClass} onClick={onPick}>
              {l.label}
            </NavLink>
          </li>
        ))}
        {authenticated && (
          <li>
            <button type="button" className={logoutClass} disabled={loggingOut} onClick={() => void handleLogout()}>
              {loggingOut ? 'Logging out…' : 'Log out'}
            </button>
          </li>
        )}
      </>
    )
  }

  return (
    <header className="border-b border-gray-200 dark:border-gray-800">
      <div className="mx-auto flex max-w-5xl items-center justify-between px-4 py-3 sm:px-6">
        <Link to="/" className="text-xl font-bold tracking-tight text-brand">
          Matcha
        </Link>

        <nav className="hidden sm:block" aria-label="Main">
          <ul className="flex gap-1">{navItems()}</ul>
        </nav>

        <button
          type="button"
          className="rounded-md p-2 text-gray-600 hover:bg-gray-100 sm:hidden dark:text-gray-300 dark:hover:bg-gray-800"
          aria-label="Toggle menu"
          aria-expanded={open}
          aria-controls="mobile-nav"
          onClick={() => setOpen((o) => !o)}
        >
          <svg className="size-6" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" aria-hidden="true">
            {open ? <path d="M6 6l12 12M18 6L6 18" /> : <path d="M4 7h16M4 12h16M4 17h16" />}
          </svg>
        </button>
      </div>

      {open && (
        <nav id="mobile-nav" className="border-t border-gray-200 px-4 py-2 sm:hidden dark:border-gray-800" aria-label="Main">
          <ul className="flex flex-col gap-1">{navItems(() => setOpen(false))}</ul>
        </nav>
      )}

      {logoutError && (
        <p role="alert" className="mx-auto max-w-5xl px-4 pb-3 text-sm text-red-600 sm:px-6 dark:text-red-400">
          {logoutError}
        </p>
      )}
    </header>
  )
}

export default Header
