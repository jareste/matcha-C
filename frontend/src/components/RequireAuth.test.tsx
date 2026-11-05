import { act, render, screen, within } from '@testing-library/react'
import userEvent from '@testing-library/user-event'
import { useEffect } from 'react'
import { MemoryRouter, Route, Routes, useLocation } from 'react-router'
import { beforeEach, describe, expect, test, vi } from 'vitest'
import { apiGet } from '../lib/api.ts'
import { jsonResponse, renderAt, session, stubApi } from '../test/utils.tsx'
import AuthProvider from './AuthProvider.tsx'
import RequireAuth from './RequireAuth.tsx'

const fetchMock = vi.fn()
let urls: string[] = []
let loginMounts = 0

// Records every request (session probes included), then defers to the fake backend.
beforeEach(() => {
  fetchMock.mockReset()
  urls = []
  loginMounts = 0
  stubApi(fetchMock)
  const fakeBackend = globalThis.fetch
  vi.stubGlobal('fetch', (input: RequestInfo | URL, init?: RequestInit) => {
    urls.push(new URL(String(input)).pathname)
    return fakeBackend(input, init)
  })
})

function probes() {
  return urls.filter((u) => u === '/api/validate')
}

function LoginProbe() {
  const state = useLocation().state as { from?: string } | null
  useEffect(() => {
    loginMounts += 1
  }, [])
  return <p>login page, back to {state?.from ?? 'nowhere'}</p>
}

// Minimal app: one private route, one public route, and a login page that
// counts how many times it is reached.
function renderGuarded(path: string) {
  return render(
    <MemoryRouter initialEntries={[path]}>
      <AuthProvider>
        <Routes>
          <Route element={<RequireAuth />}>
            <Route path="private" element={<p>private page</p>} />
          </Route>
          <Route path="public" element={<p>public page</p>} />
          <Route path="login" element={<LoginProbe />} />
        </Routes>
      </AuthProvider>
    </MemoryRouter>,
  )
}

// What a page does when it loads its data and the session is gone.
function expireDuringRequests(count: number, status = 401, error = 'Invalid or expired session') {
  fetchMock.mockResolvedValue(jsonResponse(status, { error }))
  return act(async () => {
    await Promise.all(Array.from({ length: count }, () => apiGet('/api/profile/get')))
  })
}

describe('session check', () => {
  test('asks GET /api/validate once when the app starts', async () => {
    session.username = 'alice'
    renderGuarded('/private')

    expect(await screen.findByText('private page')).toBeInTheDocument()
    expect(probes()).toEqual(['/api/validate'])
  })
})

describe('route guard', () => {
  test('a refresh on a private page keeps a valid session', async () => {
    session.username = 'alice'
    renderGuarded('/private?tab=photos')

    expect(screen.getByRole('status')).toHaveTextContent('Loading')
    expect(await screen.findByText('private page')).toBeInTheDocument()
    expect(loginMounts).toBe(0)
  })

  test('sends an anonymous user to login with the full URL to come back to', async () => {
    renderGuarded('/private?tab=photos#top')

    expect(await screen.findByText('login page, back to /private?tab=photos#top')).toBeInTheDocument()
    expect(screen.queryByText('private page')).not.toBeInTheDocument()
  })
})

describe('session expiry', () => {
  test.each([
    [401, 'Invalid or expired session'],
    [403, 'Forbidden'],
  ])('a %i answer clears the session and redirects to login', async (status, error) => {
    session.username = 'alice'
    renderGuarded('/private')
    await screen.findByText('private page')

    await expireDuringRequests(1, status, error)

    expect(await screen.findByText('login page, back to /private')).toBeInTheDocument()
    expect(screen.queryByText('private page')).not.toBeInTheDocument()
  })

  test('several 401s at once redirect only once, with no new session check', async () => {
    session.username = 'alice'
    renderGuarded('/private')
    await screen.findByText('private page')

    await expireDuringRequests(3)

    expect(await screen.findByText(/login page/)).toBeInTheDocument()
    expect(loginMounts).toBe(1)
    expect(probes()).toEqual(['/api/validate'])
  })

  test('a 401 on a public page while anonymous changes nothing', async () => {
    renderGuarded('/public')
    await act(async () => {})

    await expireDuringRequests(1)

    expect(screen.getByText('public page')).toBeInTheDocument()
    expect(loginMounts).toBe(0)
  })

  test('the login page says the session expired, until the user logs in again', async () => {
    session.username = 'alice'
    renderAt('/')
    await screen.findByRole('heading', { level: 1, name: 'Home' })

    await expireDuringRequests(1)

    expect(await screen.findByRole('heading', { level: 1, name: 'Log in' })).toBeInTheDocument()
    expect(within(screen.getByRole('main')).getByRole('status')).toHaveTextContent('Your session has expired')
    expect(within(screen.getByRole('banner')).queryByRole('button', { name: 'Log out' })).not.toBeInTheDocument()

    fetchMock.mockResolvedValue(jsonResponse(200, { success: true }))
    await userEvent.type(screen.getByLabelText('Email'), 'alice@example.com')
    await userEvent.type(screen.getByLabelText('Password'), 'Wonder1and')
    await userEvent.click(screen.getByRole('button', { name: 'Log in' }))

    expect(await screen.findByRole('heading', { level: 1, name: 'Home' })).toBeInTheDocument()
    fetchMock.mockResolvedValue(new Response(null, { status: 200 }))
    await userEvent.click(within(screen.getByRole('banner')).getByRole('button', { name: 'Log out' }))

    expect(await screen.findByRole('heading', { level: 1, name: 'Log in' })).toBeInTheDocument()
    expect(screen.queryByText(/session has expired/)).not.toBeInTheDocument()
  })
})
