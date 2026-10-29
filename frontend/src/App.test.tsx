import { screen, within } from '@testing-library/react'
import userEvent from '@testing-library/user-event'
import { describe, expect, test } from 'vitest'
import { renderAt, session } from './test/utils.tsx'

describe('routes', () => {
  test.each([
    ['/login', 'Log in'],
    ['/register', 'Sign up'],
  ])('%s renders inside the shared layout', (path, heading) => {
    renderAt(path)
    expect(screen.getByRole('heading', { level: 1, name: heading })).toBeInTheDocument()
    expect(screen.getByRole('banner')).toBeInTheDocument()
    expect(screen.getByRole('main')).toBeInTheDocument()
    expect(screen.getByRole('contentinfo')).toBeInTheDocument()
  })

  test('/ renders the private home page for a logged-in user', async () => {
    session.username = 'alice'
    renderAt('/')
    expect(await screen.findByRole('heading', { level: 1, name: 'Home' })).toBeInTheDocument()
    expect(screen.getByText(/Welcome, alice/)).toBeInTheDocument()
  })

  test('/ redirects an anonymous visitor to the login page', async () => {
    renderAt('/')
    expect(await screen.findByRole('heading', { level: 1, name: 'Log in' })).toBeInTheDocument()
    expect(screen.queryByRole('heading', { level: 1, name: 'Home' })).not.toBeInTheDocument()
  })

  test('unknown URL shows the not-found page', () => {
    renderAt('/does-not-exist')
    expect(screen.getByRole('heading', { name: 'Page not found' })).toBeInTheDocument()
    expect(screen.getByRole('banner')).toBeInTheDocument()
  })
})

describe('navigation', () => {
  test('login page links to register', async () => {
    renderAt('/login')
    const main = screen.getByRole('main')
    await userEvent.click(within(main).getByRole('link', { name: 'Sign up' }))
    expect(screen.getByRole('heading', { level: 1, name: 'Sign up' })).toBeInTheDocument()
  })

  test('register page links to login', async () => {
    renderAt('/register')
    const main = screen.getByRole('main')
    await userEvent.click(within(main).getByRole('link', { name: 'Log in' }))
    expect(screen.getByRole('heading', { level: 1, name: 'Log in' })).toBeInTheDocument()
  })

  test('not-found page links back home', async () => {
    session.username = 'alice'
    renderAt('/nope')
    await userEvent.click(screen.getByRole('link', { name: 'Back to home' }))
    expect(await screen.findByRole('heading', { level: 1, name: 'Home' })).toBeInTheDocument()
  })

  test('header link marks the current page as active', async () => {
    renderAt('/login')
    const header = screen.getByRole('banner')
    expect(await within(header).findByRole('link', { name: 'Log in' })).toHaveAttribute('aria-current', 'page')
    expect(within(header).getByRole('link', { name: 'Sign up' })).not.toHaveAttribute('aria-current')
  })

  test('header shows public links to visitors and private ones to logged-in users', async () => {
    session.username = 'alice'
    renderAt('/')
    const header = screen.getByRole('banner')
    expect(await within(header).findByRole('button', { name: 'Log out' })).toBeInTheDocument()
    expect(within(header).getByRole('link', { name: 'Home' })).toBeInTheDocument()
    expect(within(header).queryByRole('link', { name: 'Log in' })).not.toBeInTheDocument()
    expect(within(header).queryByRole('link', { name: 'Sign up' })).not.toBeInTheDocument()
  })
})

describe('mobile menu', () => {
  test('toggles open and closes after picking a link', async () => {
    renderAt('/login')
    const toggle = screen.getByRole('button', { name: 'Toggle menu' })
    expect(toggle).toHaveAttribute('aria-expanded', 'false')
    expect(document.getElementById('mobile-nav')).toBeNull()

    await userEvent.click(toggle)
    expect(toggle).toHaveAttribute('aria-expanded', 'true')
    const mobileNav = document.getElementById('mobile-nav')!
    await userEvent.click(await within(mobileNav).findByRole('link', { name: 'Sign up' }))

    expect(screen.getByRole('heading', { level: 1, name: 'Sign up' })).toBeInTheDocument()
    expect(document.getElementById('mobile-nav')).toBeNull()
  })
})
