import { screen, within } from '@testing-library/react'
import userEvent from '@testing-library/user-event'
import { describe, expect, test } from 'vitest'
import { renderAt } from './test/utils.tsx'

describe('routes', () => {
  test.each([
    ['/', 'Home'],
    ['/login', 'Log in'],
    ['/register', 'Sign up'],
  ])('%s renders inside the shared layout', (path, heading) => {
    renderAt(path)
    expect(screen.getByRole('heading', { level: 1, name: heading })).toBeInTheDocument()
    expect(screen.getByRole('banner')).toBeInTheDocument()
    expect(screen.getByRole('main')).toBeInTheDocument()
    expect(screen.getByRole('contentinfo')).toBeInTheDocument()
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
    renderAt('/nope')
    await userEvent.click(screen.getByRole('link', { name: 'Back to home' }))
    expect(screen.getByRole('heading', { level: 1, name: 'Home' })).toBeInTheDocument()
  })

  test('header link marks the current page as active', () => {
    renderAt('/login')
    const header = screen.getByRole('banner')
    expect(within(header).getByRole('link', { name: 'Log in' })).toHaveAttribute('aria-current', 'page')
    expect(within(header).getByRole('link', { name: 'Home' })).not.toHaveAttribute('aria-current')
  })
})

describe('mobile menu', () => {
  test('toggles open and closes after picking a link', async () => {
    renderAt('/')
    const toggle = screen.getByRole('button', { name: 'Toggle menu' })
    expect(toggle).toHaveAttribute('aria-expanded', 'false')
    expect(document.getElementById('mobile-nav')).toBeNull()

    await userEvent.click(toggle)
    expect(toggle).toHaveAttribute('aria-expanded', 'true')
    const mobileNav = document.getElementById('mobile-nav')!
    await userEvent.click(within(mobileNav).getByRole('link', { name: 'Sign up' }))

    expect(screen.getByRole('heading', { level: 1, name: 'Sign up' })).toBeInTheDocument()
    expect(document.getElementById('mobile-nav')).toBeNull()
  })
})
