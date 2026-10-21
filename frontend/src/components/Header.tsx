import { useState } from 'react'
import { Link, NavLink } from 'react-router'

const links = [
  { to: '/', label: 'Home' },
  { to: '/login', label: 'Log in' },
  { to: '/register', label: 'Sign up' },
]

function navClass({ isActive }: { isActive: boolean }) {
  return [
    'block rounded-md px-3 py-2 text-sm font-medium transition-colors',
    isActive
      ? 'bg-brand/10 text-brand'
      : 'text-gray-600 hover:bg-gray-100 hover:text-gray-900 dark:text-gray-300 dark:hover:bg-gray-800 dark:hover:text-white',
  ].join(' ')
}

function Header() {
  const [open, setOpen] = useState(false)

  return (
    <header className="border-b border-gray-200 dark:border-gray-800">
      <div className="mx-auto flex max-w-5xl items-center justify-between px-4 py-3 sm:px-6">
        <Link to="/" className="text-xl font-bold tracking-tight text-brand">
          Matcha
        </Link>

        <nav className="hidden sm:block" aria-label="Main">
          <ul className="flex gap-1">
            {links.map((l) => (
              <li key={l.to}>
                <NavLink to={l.to} end className={navClass}>
                  {l.label}
                </NavLink>
              </li>
            ))}
          </ul>
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
          <ul className="flex flex-col gap-1">
            {links.map((l) => (
              <li key={l.to}>
                <NavLink to={l.to} end className={navClass} onClick={() => setOpen(false)}>
                  {l.label}
                </NavLink>
              </li>
            ))}
          </ul>
        </nav>
      )}
    </header>
  )
}

export default Header
